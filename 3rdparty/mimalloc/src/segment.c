/* ----------------------------------------------------------------------------
Copyright (c) 2018, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#include "mimalloc.h"
#include "mimalloc-internal.h"
#include "mimalloc-atomic.h"

#include <string.h>  // memset
#include <stdio.h>

#define MI_PAGE_HUGE_ALIGN  (256*1024)

static void mi_segment_delayed_decommit(mi_segment_t* segment, bool force, mi_stats_t* stats);

/* --------------------------------------------------------------------------------
  Segment allocation

  In any case the memory for a segment is virtual and usually committed on demand.
  (i.e. we are careful to not touch the memory until we actually allocate a block there)

  If a  thread ends, it "abandons" pages with used blocks
  and there is an abandoned segment list whose segments can
  be reclaimed by still running threads, much like work-stealing.
-------------------------------------------------------------------------------- */

/* -----------------------------------------------------------
   Slices
----------------------------------------------------------- */


static const mi_slice_t* mi_segment_slices_end(const mi_segment_t* segment) {
  return &segment->slices[segment->slice_entries];
}

static uint8_t* mi_slice_start(const mi_slice_t* slice) {
  mi_segment_t* segment = _mi_ptr_segment(slice);
  mi_assert_internal(slice >= segment->slices && slice < mi_segment_slices_end(segment));
  return ((uint8_t*)segment + ((slice - segment->slices)*MI_SEGMENT_SLICE_SIZE));
}


/* -----------------------------------------------------------
   Bins
----------------------------------------------------------- */
// Use bit scan forward to quickly find the first zero bit if it is available

static inline size_t mi_slice_bin8(size_t slice_count) {
  if (slice_count<=1) return slice_count;
  mi_assert_internal(slice_count <= MI_SLICES_PER_SEGMENT);
  slice_count--;
  size_t s = mi_bsr(slice_count);  // slice_count > 1
  if (s <= 2) return slice_count + 1;
  size_t bin = ((s << 2) | ((slice_count >> (s - 2))&0x03)) - 4;
  return bin;
}

static inline size_t mi_slice_bin(size_t slice_count) {
  mi_assert_internal(slice_count*MI_SEGMENT_SLICE_SIZE <= MI_SEGMENT_SIZE);
  mi_assert_internal(mi_slice_bin8(MI_SLICES_PER_SEGMENT) <= MI_SEGMENT_BIN_MAX);
  size_t bin = mi_slice_bin8(slice_count);
  mi_assert_internal(bin <= MI_SEGMENT_BIN_MAX);
  return bin;
}

static inline size_t mi_slice_index(const mi_slice_t* slice) {
  mi_segment_t* segment = _mi_ptr_segment(slice);
  ptrdiff_t index = slice - segment->slices;
  mi_assert_internal(index >= 0 && index < (ptrdiff_t)segment->slice_entries);
  return index;
}


/* -----------------------------------------------------------
   Slice span queues
----------------------------------------------------------- */

static void mi_span_queue_push(mi_span_queue_t* sq, mi_slice_t* slice) {
  // todo: or push to the end?
  mi_assert_internal(slice->prev == NULL && slice->next==NULL);
  slice->prev = NULL; // paranoia
  slice->next = sq->first;
  sq->first = slice;
  if (slice->next != NULL) slice->next->prev = slice;
                     else sq->last = slice;
  slice->xblock_size = 0; // free
}

static mi_span_queue_t* mi_span_queue_for(size_t slice_count, mi_segments_tld_t* tld) {
  size_t bin = mi_slice_bin(slice_count);
  mi_span_queue_t* sq = &tld->spans[bin];
  mi_assert_internal(sq->slice_count >= slice_count);
  return sq;
}

static void mi_span_queue_delete(mi_span_queue_t* sq, mi_slice_t* slice) {
  mi_assert_internal(slice->xblock_size==0 && slice->slice_count>0 && slice->slice_offset==0);
  // should work too if the queue does not contain slice (which can happen during reclaim)
  if (slice->prev != NULL) slice->prev->next = slice->next;
  if (slice == sq->first) sq->first = slice->next;
  if (slice->next != NULL) slice->next->prev = slice->prev;
  if (slice == sq->last) sq->last = slice->prev;
  slice->prev = NULL;
  slice->next = NULL;
  slice->xblock_size = 1; // no more free
}


/* -----------------------------------------------------------
 Invariant checking
----------------------------------------------------------- */

static bool mi_slice_is_used(const mi_slice_t* slice) {
  return (slice->xblock_size > 0);
}


#if (MI_DEBUG>=3)
static bool mi_span_queue_contains(mi_span_queue_t* sq, mi_slice_t* slice) {
  for (mi_slice_t* s = sq->first; s != NULL; s = s->next) {
    if (s==slice) return true;
  }
  return false;
}

static bool mi_segment_is_valid(mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_assert_internal(segment != NULL);
  mi_assert_internal(_mi_ptr_cookie(segment) == segment->cookie);
  mi_assert_internal(segment->abandoned <= segment->used);
  mi_assert_internal(segment->thread_id == 0 || segment->thread_id == _mi_thread_id());
  mi_assert_internal(mi_commit_mask_all_set(segment->commit_mask, segment->decommit_mask)); // can only decommit committed blocks
  //mi_assert_internal(segment->segment_info_size % MI_SEGMENT_SLICE_SIZE == 0);
  mi_slice_t* slice = &segment->slices[0];
  const mi_slice_t* end = mi_segment_slices_end(segment);
  size_t used_count = 0;
  mi_span_queue_t* sq;
  while(slice < end) {
    mi_assert_internal(slice->slice_count > 0);
    mi_assert_internal(slice->slice_offset == 0);
    size_t index = mi_slice_index(slice);
    size_t maxindex = (index + slice->slice_count >= segment->slice_entries ? segment->slice_entries : index + slice->slice_count) - 1;
    if (mi_slice_is_used(slice)) { // a page in use, we need at least MAX_SLICE_OFFSET valid back offsets
      used_count++;
      for (size_t i = 0; i <= MI_MAX_SLICE_OFFSET && index + i <= maxindex; i++) {
        mi_assert_internal(segment->slices[index + i].slice_offset == i*sizeof(mi_slice_t));
        mi_assert_internal(i==0 || segment->slices[index + i].slice_count == 0);
        mi_assert_internal(i==0 || segment->slices[index + i].xblock_size == 1);
      }
      // and the last entry as well (for coalescing)
      const mi_slice_t* last = slice + slice->slice_count - 1;
      if (last > slice && last < mi_segment_slices_end(segment)) {
        mi_assert_internal(last->slice_offset == (slice->slice_count-1)*sizeof(mi_slice_t));
        mi_assert_internal(last->slice_count == 0);
        mi_assert_internal(last->xblock_size == 1);
      }
    }
    else {  // free range of slices; only last slice needs a valid back offset
      mi_slice_t* last = &segment->slices[maxindex];
      if (segment->kind != MI_SEGMENT_HUGE || slice->slice_count <= (segment->slice_entries - segment->segment_info_slices)) {
        mi_assert_internal((uint8_t*)slice == (uint8_t*)last - last->slice_offset);
      }
      mi_assert_internal(slice == last || last->slice_count == 0 );
      mi_assert_internal(last->xblock_size == 0 || (segment->kind==MI_SEGMENT_HUGE && last->xblock_size==1));
      if (segment->kind != MI_SEGMENT_HUGE && segment->thread_id != 0) { // segment is not huge or abandoned
        sq = mi_span_queue_for(slice->slice_count,tld);
        mi_assert_internal(mi_span_queue_contains(sq,slice));
      }
    }
    slice = &segment->slices[maxindex+1];
  }
  mi_assert_internal(slice == end);
  mi_assert_internal(used_count == segment->used + 1);
  return true;
}
#endif

/* -----------------------------------------------------------
 Segment size calculations
----------------------------------------------------------- */

static size_t mi_segment_info_size(mi_segment_t* segment) {
  return segment->segment_info_slices * MI_SEGMENT_SLICE_SIZE;
}

static uint8_t* _mi_segment_page_start_from_slice(const mi_segment_t* segment, const mi_slice_t* slice, size_t* page_size)
{
  ptrdiff_t idx = slice - segment->slices;
  size_t psize = slice->slice_count*MI_SEGMENT_SLICE_SIZE;
  if (page_size != NULL) *page_size = psize;
  return (uint8_t*)segment + (idx*MI_SEGMENT_SLICE_SIZE);
}

// Start of the page available memory; can be used on uninitialized pages
uint8_t* _mi_segment_page_start(const mi_segment_t* segment, const mi_page_t* page, size_t* page_size)
{
  const mi_slice_t* slice = mi_page_to_slice((mi_page_t*)page);
  uint8_t* p = _mi_segment_page_start_from_slice(segment, slice, page_size);
  mi_assert_internal(page->xblock_size == 0 || _mi_ptr_page(p) == page);
  mi_assert_internal(_mi_ptr_segment(p) == segment);
  return p;
}


static size_t mi_segment_calculate_slices(size_t required, size_t* pre_size, size_t* info_slices) {
  size_t page_size = _mi_os_page_size();
  size_t isize     = _mi_align_up(sizeof(mi_segment_t), page_size);
  size_t guardsize = 0;

  if (MI_SECURE>0) {
    // in secure mode, we set up a protected page in between the segment info
    // and the page data (and one at the end of the segment)
    guardsize =  page_size;
    required  = _mi_align_up(required, page_size);
  }

  if (pre_size != NULL) *pre_size = isize;
  isize = _mi_align_up(isize + guardsize, MI_SEGMENT_SLICE_SIZE);
  if (info_slices != NULL) *info_slices = isize / MI_SEGMENT_SLICE_SIZE;
  size_t segment_size = (required==0 ? MI_SEGMENT_SIZE : _mi_align_up( required + isize + guardsize, MI_SEGMENT_SLICE_SIZE) );  
  mi_assert_internal(segment_size % MI_SEGMENT_SLICE_SIZE == 0);
  return (segment_size / MI_SEGMENT_SLICE_SIZE);
}


/* ----------------------------------------------------------------------------
Segment caches
We keep a small segment cache per thread to increase local
reuse and avoid setting/clearing guard pages in secure mode.
------------------------------------------------------------------------------- */

static void mi_segments_track_size(long segment_size, mi_segments_tld_t* tld) {
  if (segment_size>=0) _mi_stat_increase(&tld->stats->segments,1);
                  else _mi_stat_decrease(&tld->stats->segments,1);
  tld->count += (segment_size >= 0 ? 1 : -1);
  if (tld->count > tld->peak_count) tld->peak_count = tld->count;
  tld->current_size += segment_size;
  if (tld->current_size > tld->peak_size) tld->peak_size = tld->current_size;
}

static void mi_segment_os_free(mi_segment_t* segment, mi_segments_tld_t* tld) {
  segment->thread_id = 0;
  _mi_segment_map_freed_at(segment);
  mi_segments_track_size(-((long)mi_segment_size(segment)),tld);
  if (MI_SECURE>0) {
    // _mi_os_unprotect(segment, mi_segment_size(segment)); // ensure no more guard pages are set
    // unprotect the guard pages; we cannot just unprotect the whole segment size as part may be decommitted
    size_t os_pagesize = _mi_os_page_size();
    _mi_os_unprotect((uint8_t*)segment + mi_segment_info_size(segment) - os_pagesize, os_pagesize);
    uint8_t* end = (uint8_t*)segment + mi_segment_size(segment) - os_pagesize;
    _mi_os_unprotect(end, os_pagesize);
  }

  // purge delayed decommits now? (no, leave it to the cache)
  // mi_segment_delayed_decommit(segment,true,tld->stats);
  
  // _mi_os_free(segment, mi_segment_size(segment), /*segment->memid,*/ tld->stats);
  const size_t size = mi_segment_size(segment);
  if (size != MI_SEGMENT_SIZE || !_mi_segment_cache_push(segment, size, segment->memid, segment->commit_mask, segment->mem_is_large, segment->mem_is_pinned, tld->os)) {
    const size_t csize = mi_commit_mask_committed_size(segment->commit_mask, size);
    if (csize > 0 && !segment->mem_is_pinned) _mi_stat_decrease(&_mi_stats_main.committed, csize);
    _mi_abandoned_await_readers();  // wait until safe to free
    _mi_arena_free(segment, mi_segment_size(segment), segment->memid, segment->mem_is_pinned /* pretend not committed to not double count decommits */, tld->os);
  }
}


// The thread local segment cache is limited to be at most 1/8 of the peak size of segments in use,
#define MI_SEGMENT_CACHE_FRACTION (8)

// note: returned segment may be partially reset
static mi_segment_t* mi_segment_cache_pop(size_t segment_slices, mi_segments_tld_t* tld) {
  if (segment_slices != 0 && segment_slices != MI_SLICES_PER_SEGMENT) return NULL;
  mi_segment_t* segment = tld->cache;
  if (segment == NULL) return NULL;
  tld->cache_count--;
  tld->cache = segment->next;
  segment->next = NULL;
  mi_assert_internal(segment->segment_slices == MI_SLICES_PER_SEGMENT);
  _mi_stat_decrease(&tld->stats->segments_cache, 1);
  return segment;
}

static bool mi_segment_cache_full(mi_segments_tld_t* tld)
{
  // if (tld->count == 1 && tld->cache_count==0) return false; // always cache at least the final segment of a thread
  size_t max_cache = mi_option_get(mi_option_segment_cache);
  if (tld->cache_count < max_cache
       && tld->cache_count < (1 + (tld->peak_count / MI_SEGMENT_CACHE_FRACTION)) // at least allow a 1 element cache
     ) {
    return false;
  }
  // take the opportunity to reduce the segment cache if it is too large (now)
  // TODO: this never happens as we check against peak usage, should we use current usage instead?
  while (tld->cache_count > max_cache) { //(1 + (tld->peak_count / MI_SEGMENT_CACHE_FRACTION))) {
    mi_segment_t* segment = mi_segment_cache_pop(0,tld);
    mi_assert_internal(segment != NULL);
    if (segment != NULL) mi_segment_os_free(segment, tld);
  }
  return true;
}

static bool mi_segment_cache_push(mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_assert_internal(segment->next == NULL);
  if (segment->segment_slices != MI_SLICES_PER_SEGMENT || mi_segment_cache_full(tld)) {
    return false;
  }
  // mi_segment_delayed_decommit(segment, true, tld->stats);  
  mi_assert_internal(segment->segment_slices == MI_SLICES_PER_SEGMENT);
  mi_assert_internal(segment->next == NULL);  
  segment->next = tld->cache;
  tld->cache = segment;
  tld->cache_count++;
  _mi_stat_increase(&tld->stats->segments_cache,1);
  return true;
}

// called by threads that are terminating to free cached segments
void _mi_segment_thread_collect(mi_segments_tld_t* tld) {
  mi_segment_t* segment;
  while ((segment = mi_segment_cache_pop(0,tld)) != NULL) {
    mi_segment_os_free(segment, tld);
  }
  mi_assert_internal(tld->cache_count == 0);
  mi_assert_internal(tld->cache == NULL);  
}



/* -----------------------------------------------------------
   Span management
----------------------------------------------------------- */

static mi_commit_mask_t mi_segment_commit_mask(mi_segment_t* segment, bool conservative, uint8_t* p, size_t size, uint8_t** start_p, size_t* full_size) {
  mi_assert_internal(_mi_ptr_segment(p) == segment);
  if (size == 0 || size > MI_SEGMENT_SIZE) return 0;
  if (p >= (uint8_t*)segment + mi_segment_size(segment)) return 0;

  uintptr_t diff = (p - (uint8_t*)segment);
  uintptr_t start;
  uintptr_t end;
  if (conservative) {
    start = _mi_align_up(diff, MI_COMMIT_SIZE);
    end   = _mi_align_down(diff + size, MI_COMMIT_SIZE);
  }
  else {
    start = _mi_align_down(diff, MI_COMMIT_SIZE);
    end   = _mi_align_up(diff + size, MI_COMMIT_SIZE);
  }

  mi_assert_internal(start % MI_COMMIT_SIZE==0 && end % MI_COMMIT_SIZE == 0);
  *start_p   = (uint8_t*)segment + start;
  *full_size = (end > start ? end - start : 0);
  if (*full_size == 0) return 0;

  uintptr_t bitidx = start / MI_COMMIT_SIZE;
  mi_assert_internal(bitidx < (MI_INTPTR_SIZE*8));
  
  uintptr_t bitcount = *full_size / MI_COMMIT_SIZE; // can be 0
  if (bitidx + bitcount > MI_INTPTR_SIZE*8) {
    _mi_warning_message("commit mask overflow: %zu %zu %zu %zu 0x%p %zu\n", bitidx, bitcount, start, end, p, size);
  }
  mi_assert_internal((bitidx + bitcount) <= (MI_INTPTR_SIZE*8));

  return mi_commit_mask_create(bitidx, bitcount);
}

static bool mi_segment_commitx(mi_segment_t* segment, bool commit, uint8_t* p, size_t size, mi_stats_t* stats) {    
  // commit liberal, but decommit conservative
  uint8_t* start;
  size_t   full_size;
  mi_commit_mask_t mask = mi_segment_commit_mask(segment,!commit/*conservative*/,p,size,&start,&full_size);  
  if (mi_commit_mask_is_empty(mask) || full_size==0) return true;

  if (commit && !mi_commit_mask_all_set(segment->commit_mask, mask)) {
    bool is_zero = false;
    mi_commit_mask_t cmask = mi_commit_mask_intersect(segment->commit_mask, mask);
    _mi_stat_decrease(&_mi_stats_main.committed, mi_commit_mask_committed_size(cmask, MI_SEGMENT_SIZE)); // adjust for overlap
    if (!_mi_os_commit(start,full_size,&is_zero,stats)) return false;    
    mi_commit_mask_set(&segment->commit_mask,mask);     
  }
  else if (!commit && mi_commit_mask_any_set(segment->commit_mask,mask)) {
    mi_assert_internal((void*)start != (void*)segment);
    mi_commit_mask_t cmask = mi_commit_mask_intersect(segment->commit_mask, mask);
    _mi_stat_increase(&_mi_stats_main.committed, full_size - mi_commit_mask_committed_size(cmask, MI_SEGMENT_SIZE)); // adjust for overlap
    if (segment->allow_decommit) { _mi_os_decommit(start, full_size, stats); } // ok if this fails
    mi_commit_mask_clear(&segment->commit_mask, mask);
  }
  // increase expiration of reusing part of the delayed decommit
  if (commit && mi_commit_mask_any_set(segment->decommit_mask, mask)) {
    segment->decommit_expire = _mi_clock_now() + mi_option_get(mi_option_reset_delay);
  }
  // always undo delayed decommits
  mi_commit_mask_clear(&segment->decommit_mask, mask);   
  mi_assert_internal((segment->commit_mask & segment->decommit_mask) == segment->decommit_mask);
  return true;
}

static bool mi_segment_ensure_committed(mi_segment_t* segment, uint8_t* p, size_t size, mi_stats_t* stats) {
  mi_assert_internal(mi_commit_mask_all_set(segment->commit_mask, segment->decommit_mask));
  if (mi_commit_mask_is_full(segment->commit_mask) && mi_commit_mask_is_empty(segment->decommit_mask)) return true; // fully committed
  return mi_segment_commitx(segment,true,p,size,stats);
}

static void mi_segment_perhaps_decommit(mi_segment_t* segment, uint8_t* p, size_t size, mi_stats_t* stats) {
  if (!segment->allow_decommit) return;
  if (mi_option_get(mi_option_reset_delay) == 0) {
    mi_segment_commitx(segment, false, p, size, stats);
  }
  else {
    // register for future decommit in the decommit mask
    uint8_t* start;
    size_t   full_size;
    mi_commit_mask_t mask = mi_segment_commit_mask(segment, true /*conservative*/, p, size, &start, &full_size);
    if (mi_commit_mask_is_empty(mask) || full_size==0) return;
    
    // update delayed commit
    mi_commit_mask_set(&segment->decommit_mask, mi_commit_mask_intersect(mask,segment->commit_mask));  // only decommit what is committed; span_free may try to decommit more
    segment->decommit_expire = _mi_clock_now() + mi_option_get(mi_option_reset_delay);
  }  
}

static void mi_segment_delayed_decommit(mi_segment_t* segment, bool force, mi_stats_t* stats) {
  if (!segment->allow_decommit || mi_commit_mask_is_empty(segment->decommit_mask)) return;
  mi_msecs_t now = _mi_clock_now();
  if (!force && now < segment->decommit_expire) return;

  mi_commit_mask_t mask = segment->decommit_mask;
  segment->decommit_expire = 0;
  segment->decommit_mask = mi_commit_mask_empty();

  uintptr_t idx;
  uintptr_t count;
  mi_commit_mask_foreach(mask, idx, count) {
    // if found, decommit that sequence
    if (count > 0) {
      uint8_t* p = (uint8_t*)segment + (idx*MI_COMMIT_SIZE);
      size_t size = count * MI_COMMIT_SIZE;
      mi_segment_commitx(segment, false, p, size, stats);
    }
  }
  mi_commit_mask_foreach_end()
  mi_assert_internal(mi_commit_mask_is_empty(segment->decommit_mask));
}


static bool mi_segment_is_abandoned(mi_segment_t* segment) {
  return (segment->thread_id == 0);
}

// note: can be called on abandoned segments
static void mi_segment_span_free(mi_segment_t* segment, size_t slice_index, size_t slice_count, mi_segments_tld_t* tld) {
  mi_assert_internal(slice_index < segment->slice_entries);
  mi_span_queue_t* sq = (segment->kind == MI_SEGMENT_HUGE || mi_segment_is_abandoned(segment) 
                          ? NULL : mi_span_queue_for(slice_count,tld));
  if (slice_count==0) slice_count = 1;
  mi_assert_internal(slice_index + slice_count - 1 < segment->slice_entries);

  // set first and last slice (the intermediates can be undetermined)
  mi_slice_t* slice = &segment->slices[slice_index];
  slice->slice_count = (uint32_t)slice_count;
  mi_assert_internal(slice->slice_count == slice_count); // no overflow?
  slice->slice_offset = 0;
  if (slice_count > 1) {
    mi_slice_t* last = &segment->slices[slice_index + slice_count - 1];
    last->slice_count = 0;
    last->slice_offset = (uint32_t)(sizeof(mi_page_t)*(slice_count - 1));
    last->xblock_size = 0;
  }

  // perhaps decommit
  mi_segment_perhaps_decommit(segment,mi_slice_start(slice),slice_count*MI_SEGMENT_SLICE_SIZE,tld->stats);
  
  // and push it on the free page queue (if it was not a huge page)
  if (sq != NULL) mi_span_queue_push( sq, slice );
             else slice->xblock_size = 0; // mark huge page as free anyways
}

/*
// called from reclaim to add existing free spans
static void mi_segment_span_add_free(mi_slice_t* slice, mi_segments_tld_t* tld) {
  mi_segment_t* segment = _mi_ptr_segment(slice);
  mi_assert_internal(slice->xblock_size==0 && slice->slice_count>0 && slice->slice_offset==0);
  size_t slice_index = mi_slice_index(slice);
  mi_segment_span_free(segment,slice_index,slice->slice_count,tld);
}
*/

static void mi_segment_span_remove_from_queue(mi_slice_t* slice, mi_segments_tld_t* tld) {
  mi_assert_internal(slice->slice_count > 0 && slice->slice_offset==0 && slice->xblock_size==0);
  mi_assert_internal(_mi_ptr_segment(slice)->kind != MI_SEGMENT_HUGE);
  mi_span_queue_t* sq = mi_span_queue_for(slice->slice_count, tld);
  mi_span_queue_delete(sq, slice);
}

// note: can be called on abandoned segments
static mi_slice_t* mi_segment_span_free_coalesce(mi_slice_t* slice, mi_segments_tld_t* tld) {
  mi_assert_internal(slice != NULL && slice->slice_count > 0 && slice->slice_offset == 0);
  mi_segment_t* segment = _mi_ptr_segment(slice);
  bool is_abandoned = mi_segment_is_abandoned(segment);

  // for huge pages, just mark as free but don't add to the queues
  if (segment->kind == MI_SEGMENT_HUGE) {
    mi_assert_internal(segment->used == 1);  // decreased right after this call in `mi_segment_page_clear`
    slice->xblock_size = 0;  // mark as free anyways
    // we should mark the last slice `xblock_size=0` now to maintain invariants but we skip it to 
    // avoid a possible cache miss (and the segment is about to be freed)
    return slice;
  }

  // otherwise coalesce the span and add to the free span queues
  size_t slice_count = slice->slice_count;
  mi_slice_t* next = slice + slice->slice_count;
  mi_assert_internal(next <= mi_segment_slices_end(segment));
  if (next < mi_segment_slices_end(segment) && next->xblock_size==0) {
    // free next block -- remove it from free and merge
    mi_assert_internal(next->slice_count > 0 && next->slice_offset==0);
    slice_count += next->slice_count; // extend
    if (!is_abandoned) { mi_segment_span_remove_from_queue(next, tld); }
  }
  if (slice > segment->slices) {
    mi_slice_t* prev = mi_slice_first(slice - 1);
    mi_assert_internal(prev >= segment->slices);
    if (prev->xblock_size==0) {
      // free previous slice -- remove it from free and merge
      mi_assert_internal(prev->slice_count > 0 && prev->slice_offset==0);
      slice_count += prev->slice_count;
      if (!is_abandoned) { mi_segment_span_remove_from_queue(prev, tld); }
      slice = prev;
    }
  }

  // and add the new free page
  mi_segment_span_free(segment, mi_slice_index(slice), slice_count, tld);
  return slice;
}


static void mi_segment_slice_split(mi_segment_t* segment, mi_slice_t* slice, size_t slice_count, mi_segments_tld_t* tld) {
  mi_assert_internal(_mi_ptr_segment(slice)==segment);
  mi_assert_internal(slice->slice_count >= slice_count);
  mi_assert_internal(slice->xblock_size > 0); // no more in free queue
  if (slice->slice_count <= slice_count) return;
  mi_assert_internal(segment->kind != MI_SEGMENT_HUGE);
  size_t next_index = mi_slice_index(slice) + slice_count;
  size_t next_count = slice->slice_count - slice_count;
  mi_segment_span_free(segment, next_index, next_count, tld);
  slice->slice_count = (uint32_t)slice_count;
}

// Note: may still return NULL if committing the memory failed
static mi_page_t* mi_segment_span_allocate(mi_segment_t* segment, size_t slice_index, size_t slice_count, mi_segments_tld_t* tld) {
  mi_assert_internal(slice_index < segment->slice_entries);
  mi_slice_t* slice = &segment->slices[slice_index];
  mi_assert_internal(slice->xblock_size==0 || slice->xblock_size==1);

  // commit before changing the slice data
  if (!mi_segment_ensure_committed(segment, _mi_segment_page_start_from_slice(segment, slice, NULL), slice_count * MI_SEGMENT_SLICE_SIZE, tld->stats)) {
    return NULL;  // commit failed!
  }

  // convert the slices to a page
  slice->slice_offset = 0;
  slice->slice_count = (uint32_t)slice_count;
  mi_assert_internal(slice->slice_count == slice_count);
  const size_t bsize = slice_count * MI_SEGMENT_SLICE_SIZE;
  slice->xblock_size = (uint32_t)(bsize >= MI_HUGE_BLOCK_SIZE ? MI_HUGE_BLOCK_SIZE : bsize);
  mi_page_t*  page = mi_slice_to_page(slice);
  mi_assert_internal(mi_page_block_size(page) == bsize);

  // set slice back pointers for the first MI_MAX_SLICE_OFFSET entries
  size_t extra = slice_count-1;
  if (extra > MI_MAX_SLICE_OFFSET) extra = MI_MAX_SLICE_OFFSET;
  if (slice_index + extra >= segment->slice_entries) extra = segment->slice_entries - slice_index - 1;  // huge objects may have more slices than avaiable entries in the segment->slices
  slice++;
  for (size_t i = 1; i <= extra; i++, slice++) {
    slice->slice_offset = (uint32_t)(sizeof(mi_slice_t)*i);
    slice->slice_count = 0;
    slice->xblock_size = 1;
  }

  // and also for the last one (if not set already) (the last one is needed for coalescing)
  mi_slice_t* last = &segment->slices[slice_index + slice_count - 1];
  if (last < mi_segment_slices_end(segment) && last >= slice) {
    last->slice_offset = (uint32_t)(sizeof(mi_slice_t)*(slice_count-1));
    last->slice_count = 0;
    last->xblock_size = 1;
  }
  
  // and initialize the page
  page->is_reset = false;
  page->is_committed = true;
  segment->used++;
  return page;
}

static mi_page_t* mi_segments_page_find_and_allocate(size_t slice_count, mi_segments_tld_t* tld) {
  mi_assert_internal(slice_count*MI_SEGMENT_SLICE_SIZE <= MI_LARGE_OBJ_SIZE_MAX);
  // search from best fit up
  mi_span_queue_t* sq = mi_span_queue_for(slice_count, tld);
  if (slice_count == 0) slice_count = 1;
  while (sq <= &tld->spans[MI_SEGMENT_BIN_MAX]) {
    for (mi_slice_t* slice = sq->first; slice != NULL; slice = slice->next) {
      if (slice->slice_count >= slice_count) {
        // found one
        mi_span_queue_delete(sq, slice);
        mi_segment_t* segment = _mi_ptr_segment(slice);
        if (slice->slice_count > slice_count) {
          mi_segment_slice_split(segment, slice, slice_count, tld);
        }
        mi_assert_internal(slice != NULL && slice->slice_count == slice_count && slice->xblock_size > 0);
        mi_page_t* page = mi_segment_span_allocate(segment, mi_slice_index(slice), slice->slice_count, tld);
        if (page == NULL) {
          // commit failed; return NULL but first restore the slice
          mi_segment_span_free_coalesce(slice, tld);
          return NULL;
        }
        return page;        
      }
    }
    sq++;
  }
  // could not find a page..
  return NULL;
}


/* -----------------------------------------------------------
   Segment allocation
----------------------------------------------------------- */

// Allocate a segment from the OS aligned to `MI_SEGMENT_SIZE` .
static mi_segment_t* mi_segment_init(mi_segment_t* segment, size_t required, mi_segments_tld_t* tld, mi_os_tld_t* os_tld, mi_page_t** huge_page)
{
  mi_assert_internal((required==0 && huge_page==NULL) || (required>0 && huge_page != NULL));
  mi_assert_internal((segment==NULL) || (segment!=NULL && required==0));
  // calculate needed sizes first
  size_t info_slices;
  size_t pre_size;
  const size_t segment_slices = mi_segment_calculate_slices(required, &pre_size, &info_slices);
  const size_t slice_entries = (segment_slices > MI_SLICES_PER_SEGMENT ? MI_SLICES_PER_SEGMENT : segment_slices);
  const size_t segment_size = segment_slices * MI_SEGMENT_SLICE_SIZE;

  // Commit eagerly only if not the first N lazy segments (to reduce impact of many threads that allocate just a little)
  const bool eager_delay = (tld->count < (size_t)mi_option_get(mi_option_eager_commit_delay));
  const bool eager = !eager_delay && mi_option_is_enabled(mi_option_eager_commit);
  bool commit = eager || (required > 0); 
  
  // Try to get from our cache first
  bool is_zero = false;
  const bool commit_info_still_good = (segment != NULL);
  mi_commit_mask_t commit_mask = (segment != NULL ? segment->commit_mask : mi_commit_mask_empty());
  if (segment==NULL) {
    // Allocate the segment from the OS
    bool mem_large = (!eager_delay && (MI_SECURE==0)); // only allow large OS pages once we are no longer lazy    
    bool is_pinned = false;
    size_t memid = 0;
    segment = (mi_segment_t*)_mi_segment_cache_pop(segment_size, &commit_mask, &mem_large, &is_pinned, &is_zero, &memid, os_tld);
    if (segment==NULL) {
      segment = (mi_segment_t*)_mi_arena_alloc_aligned(segment_size, MI_SEGMENT_SIZE, &commit, &mem_large, &is_pinned, &is_zero, &memid, os_tld);
      if (segment == NULL) return NULL;  // failed to allocate
      commit_mask = (commit ? mi_commit_mask_full() : mi_commit_mask_empty());
    }    
    mi_assert_internal(segment != NULL && (uintptr_t)segment % MI_SEGMENT_SIZE == 0);

    const size_t commit_needed = _mi_divide_up(info_slices*MI_SEGMENT_SLICE_SIZE, MI_COMMIT_SIZE);
    mi_assert_internal(commit_needed>0);
    if (!mi_commit_mask_all_set(commit_mask,mi_commit_mask_create(0, commit_needed))) {
      // at least commit the info slices
      mi_assert_internal(commit_needed*MI_COMMIT_SIZE > info_slices*MI_SEGMENT_SLICE_SIZE);
      bool ok = _mi_os_commit(segment, commit_needed*MI_COMMIT_SIZE, &is_zero, tld->stats);
      if (!ok) return NULL; // failed to commit   
      mi_commit_mask_set(&commit_mask,mi_commit_mask_create(0, commit_needed)); 
    }
    segment->memid = memid;
    segment->mem_is_pinned = is_pinned;
    segment->mem_is_large = mem_large;
    segment->mem_is_committed = mi_commit_mask_is_full(commit_mask);
    mi_segments_track_size((long)(segment_size), tld);
    _mi_segment_map_allocated_at(segment);
  }

  // zero the segment info? -- not always needed as it is zero initialized from the OS 
  mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, NULL);  // tsan
  if (!is_zero) {
    ptrdiff_t ofs = offsetof(mi_segment_t, next);
    size_t    prefix = offsetof(mi_segment_t, slices) - ofs;
    memset((uint8_t*)segment+ofs, 0, prefix + sizeof(mi_slice_t)*segment_slices);
  }

  if (!commit_info_still_good) {
    segment->commit_mask = commit_mask; // on lazy commit, the initial part is always committed
    segment->allow_decommit = (mi_option_is_enabled(mi_option_allow_decommit) && !segment->mem_is_pinned && !segment->mem_is_large);
    segment->decommit_expire = 0;
    segment->decommit_mask = mi_commit_mask_empty();
  }

  // initialize segment info
  segment->segment_slices = segment_slices;
  segment->segment_info_slices = info_slices;
  segment->thread_id = _mi_thread_id();
  segment->cookie = _mi_ptr_cookie(segment);
  segment->slice_entries = slice_entries;
  segment->kind = (required == 0 ? MI_SEGMENT_NORMAL : MI_SEGMENT_HUGE);

  // memset(segment->slices, 0, sizeof(mi_slice_t)*(info_slices+1));
  _mi_stat_increase(&tld->stats->page_committed, mi_segment_info_size(segment));

  // set up guard pages
  size_t guard_slices = 0;
  if (MI_SECURE>0) {
    // in secure mode, we set up a protected page in between the segment info
    // and the page data
    size_t os_pagesize = _mi_os_page_size();    
    mi_assert_internal(mi_segment_info_size(segment) - os_pagesize >= pre_size);
    _mi_os_protect((uint8_t*)segment + mi_segment_info_size(segment) - os_pagesize, os_pagesize);
    uint8_t* end = (uint8_t*)segment + mi_segment_size(segment) - os_pagesize;
    mi_segment_ensure_committed(segment, end, os_pagesize, tld->stats);
    _mi_os_protect(end, os_pagesize);
    if (slice_entries == segment_slices) segment->slice_entries--; // don't use the last slice :-(
    guard_slices = 1;
  }

  // reserve first slices for segment info
  mi_page_t* page0 = mi_segment_span_allocate(segment, 0, info_slices, tld);
  mi_assert_internal(page0!=NULL); if (page0==NULL) return NULL; // cannot fail as we always commit in advance  
  mi_assert_internal(segment->used == 1);
  segment->used = 0; // don't count our internal slices towards usage
  
  // initialize initial free pages
  if (segment->kind == MI_SEGMENT_NORMAL) { // not a huge page
    mi_assert_internal(huge_page==NULL);
    mi_segment_span_free(segment, info_slices, segment->slice_entries - info_slices, tld);
  }
  else {
    mi_assert_internal(huge_page!=NULL);
    *huge_page = mi_segment_span_allocate(segment, info_slices, segment_slices - info_slices - guard_slices, tld);
    mi_assert_internal(*huge_page != NULL); // cannot fail as we commit in advance 
  }

  mi_assert_expensive(mi_segment_is_valid(segment,tld));
  return segment;
}


// Allocate a segment from the OS aligned to `MI_SEGMENT_SIZE` .
static mi_segment_t* mi_segment_alloc(size_t required, mi_segments_tld_t* tld, mi_os_tld_t* os_tld, mi_page_t** huge_page) {
  return mi_segment_init(NULL, required, tld, os_tld, huge_page);
}


static void mi_segment_free(mi_segment_t* segment, bool force, mi_segments_tld_t* tld) {
  mi_assert_internal(segment != NULL);
  mi_assert_internal(segment->next == NULL);
  mi_assert_internal(segment->used == 0);

  // Remove the free pages
  mi_slice_t* slice = &segment->slices[0];
  const mi_slice_t* end = mi_segment_slices_end(segment);
  size_t page_count = 0;
  while (slice < end) {
    mi_assert_internal(slice->slice_count > 0);
    mi_assert_internal(slice->slice_offset == 0);
    mi_assert_internal(mi_slice_index(slice)==0 || slice->xblock_size == 0); // no more used pages ..
    if (slice->xblock_size == 0 && segment->kind != MI_SEGMENT_HUGE) {
      mi_segment_span_remove_from_queue(slice, tld);
    }
    page_count++;
    slice = slice + slice->slice_count;
  }
  mi_assert_internal(page_count == 2); // first page is allocated by the segment itself

  // stats
  _mi_stat_decrease(&tld->stats->page_committed, mi_segment_info_size(segment));

  if (!force && mi_segment_cache_push(segment, tld)) {
    // it is put in our cache
  }
  else {
    // otherwise return it to the OS
    mi_segment_os_free(segment,  tld);
  }
}


/* -----------------------------------------------------------
   Page Free
----------------------------------------------------------- */

static void mi_segment_abandon(mi_segment_t* segment, mi_segments_tld_t* tld);

// note: can be called on abandoned pages
static mi_slice_t* mi_segment_page_clear(mi_page_t* page, mi_segments_tld_t* tld) {
  mi_assert_internal(page->xblock_size > 0);
  mi_assert_internal(mi_page_all_free(page));
  mi_segment_t* segment = _mi_ptr_segment(page);
  mi_assert_internal(segment->used > 0);
  
  size_t inuse = page->capacity * mi_page_block_size(page);
  _mi_stat_decrease(&tld->stats->page_committed, inuse);
  _mi_stat_decrease(&tld->stats->pages, 1);

  // reset the page memory to reduce memory pressure?
  if (!segment->mem_is_pinned && !page->is_reset && mi_option_is_enabled(mi_option_page_reset)) {
    size_t psize;
    uint8_t* start = _mi_page_start(segment, page, &psize);
    page->is_reset = true;
    _mi_os_reset(start, psize, tld->stats);
  }

  // zero the page data, but not the segment fields
  page->is_zero_init = false;
  ptrdiff_t ofs = offsetof(mi_page_t, capacity);
  memset((uint8_t*)page + ofs, 0, sizeof(*page) - ofs);
  page->xblock_size = 1;

  // and free it
  mi_slice_t* slice = mi_segment_span_free_coalesce(mi_page_to_slice(page), tld);  
  segment->used--;
  // cannot assert segment valid as it is called during reclaim
  // mi_assert_expensive(mi_segment_is_valid(segment, tld));
  return slice;
}

void _mi_segment_page_free(mi_page_t* page, bool force, mi_segments_tld_t* tld)
{
  mi_assert(page != NULL);

  mi_segment_t* segment = _mi_page_segment(page);
  mi_assert_expensive(mi_segment_is_valid(segment,tld));

  // mark it as free now
  mi_segment_page_clear(page, tld);
  mi_assert_expensive(mi_segment_is_valid(segment, tld));

  if (segment->used == 0) {
    // no more used pages; remove from the free list and free the segment
    mi_segment_free(segment, force, tld);
  }
  else if (segment->used == segment->abandoned) {
    // only abandoned pages; remove from free list and abandon
    mi_segment_abandon(segment,tld);
  }
}


/* -----------------------------------------------------------
Abandonment

When threads terminate, they can leave segments with
live blocks (reached through other threads). Such segments
are "abandoned" and will be reclaimed by other threads to
reuse their pages and/or free them eventually

We maintain a global list of abandoned segments that are
reclaimed on demand. Since this is shared among threads
the implementation needs to avoid the A-B-A problem on
popping abandoned segments: <https://en.wikipedia.org/wiki/ABA_problem>
We use tagged pointers to avoid accidentially identifying
reused segments, much like stamped references in Java.
Secondly, we maintain a reader counter to avoid resetting
or decommitting segments that have a pending read operation.

Note: the current implementation is one possible design;
another way might be to keep track of abandoned segments
in the regions. This would have the advantage of keeping
all concurrent code in one place and not needing to deal
with ABA issues. The drawback is that it is unclear how to
scan abandoned segments efficiently in that case as they
would be spread among all other segments in the regions.
----------------------------------------------------------- */

// Use the bottom 20-bits (on 64-bit) of the aligned segment pointers
// to put in a tag that increments on update to avoid the A-B-A problem.
#define MI_TAGGED_MASK   MI_SEGMENT_MASK
typedef uintptr_t        mi_tagged_segment_t;

static mi_segment_t* mi_tagged_segment_ptr(mi_tagged_segment_t ts) {
  return (mi_segment_t*)(ts & ~MI_TAGGED_MASK);
}

static mi_tagged_segment_t mi_tagged_segment(mi_segment_t* segment, mi_tagged_segment_t ts) {
  mi_assert_internal(((uintptr_t)segment & MI_TAGGED_MASK) == 0);
  uintptr_t tag = ((ts & MI_TAGGED_MASK) + 1) & MI_TAGGED_MASK;
  return ((uintptr_t)segment | tag);
}

// This is a list of visited abandoned pages that were full at the time.
// this list migrates to `abandoned` when that becomes NULL. The use of
// this list reduces contention and the rate at which segments are visited.
static mi_decl_cache_align _Atomic(mi_segment_t*)       abandoned_visited; // = NULL

// The abandoned page list (tagged as it supports pop)
static mi_decl_cache_align _Atomic(mi_tagged_segment_t) abandoned;         // = NULL

// Maintain these for debug purposes (these counts may be a bit off)
static mi_decl_cache_align _Atomic(uintptr_t)           abandoned_count; 
static mi_decl_cache_align _Atomic(uintptr_t)           abandoned_visited_count;

// We also maintain a count of current readers of the abandoned list
// in order to prevent resetting/decommitting segment memory if it might
// still be read.
static mi_decl_cache_align _Atomic(uintptr_t)           abandoned_readers; // = 0

// Push on the visited list
static void mi_abandoned_visited_push(mi_segment_t* segment) {
  mi_assert_internal(segment->thread_id == 0);
  mi_assert_internal(mi_atomic_load_ptr_relaxed(mi_segment_t,&segment->abandoned_next) == NULL);
  mi_assert_internal(segment->next == NULL);
  mi_assert_internal(segment->used > 0);
  mi_segment_t* anext = mi_atomic_load_ptr_relaxed(mi_segment_t, &abandoned_visited);
  do {
    mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, anext);
  } while (!mi_atomic_cas_ptr_weak_release(mi_segment_t, &abandoned_visited, &anext, segment));
  mi_atomic_increment_relaxed(&abandoned_visited_count);
}

// Move the visited list to the abandoned list.
static bool mi_abandoned_visited_revisit(void)
{
  // quick check if the visited list is empty
  if (mi_atomic_load_ptr_relaxed(mi_segment_t, &abandoned_visited) == NULL) return false;

  // grab the whole visited list
  mi_segment_t* first = mi_atomic_exchange_ptr_acq_rel(mi_segment_t, &abandoned_visited, NULL);
  if (first == NULL) return false;

  // first try to swap directly if the abandoned list happens to be NULL
  mi_tagged_segment_t afirst;
  mi_tagged_segment_t ts = mi_atomic_load_relaxed(&abandoned);
  if (mi_tagged_segment_ptr(ts)==NULL) {
    uintptr_t count = mi_atomic_load_relaxed(&abandoned_visited_count);
    afirst = mi_tagged_segment(first, ts);
    if (mi_atomic_cas_strong_acq_rel(&abandoned, &ts, afirst)) {
      mi_atomic_add_relaxed(&abandoned_count, count);
      mi_atomic_sub_relaxed(&abandoned_visited_count, count);
      return true;
    }
  }

  // find the last element of the visited list: O(n)
  mi_segment_t* last = first;
  mi_segment_t* next;
  while ((next = mi_atomic_load_ptr_relaxed(mi_segment_t, &last->abandoned_next)) != NULL) {
    last = next;
  }

  // and atomically prepend to the abandoned list
  // (no need to increase the readers as we don't access the abandoned segments)
  mi_tagged_segment_t anext = mi_atomic_load_relaxed(&abandoned);
  uintptr_t count;
  do {
    count = mi_atomic_load_relaxed(&abandoned_visited_count);
    mi_atomic_store_ptr_release(mi_segment_t, &last->abandoned_next, mi_tagged_segment_ptr(anext));
    afirst = mi_tagged_segment(first, anext);
  } while (!mi_atomic_cas_weak_release(&abandoned, &anext, afirst));
  mi_atomic_add_relaxed(&abandoned_count, count);
  mi_atomic_sub_relaxed(&abandoned_visited_count, count);
  return true;
}

// Push on the abandoned list.
static void mi_abandoned_push(mi_segment_t* segment) {
  mi_assert_internal(segment->thread_id == 0);
  mi_assert_internal(mi_atomic_load_ptr_relaxed(mi_segment_t, &segment->abandoned_next) == NULL);
  mi_assert_internal(segment->next == NULL);
  mi_assert_internal(segment->used > 0);
  mi_tagged_segment_t next;
  mi_tagged_segment_t ts = mi_atomic_load_relaxed(&abandoned);
  do {
    mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, mi_tagged_segment_ptr(ts));
    next = mi_tagged_segment(segment, ts);
  } while (!mi_atomic_cas_weak_release(&abandoned, &ts, next));
  mi_atomic_increment_relaxed(&abandoned_count);
}

// Wait until there are no more pending reads on segments that used to be in the abandoned list
// called for example from `arena.c` before decommitting
void _mi_abandoned_await_readers(void) {
  uintptr_t n;
  do {
    n = mi_atomic_load_acquire(&abandoned_readers);
    if (n != 0) mi_atomic_yield();
  } while (n != 0);
}

// Pop from the abandoned list
static mi_segment_t* mi_abandoned_pop(void) {
  mi_segment_t* segment;
  // Check efficiently if it is empty (or if the visited list needs to be moved)
  mi_tagged_segment_t ts = mi_atomic_load_relaxed(&abandoned);
  segment = mi_tagged_segment_ptr(ts);
  if (mi_likely(segment == NULL)) {
    if (mi_likely(!mi_abandoned_visited_revisit())) { // try to swap in the visited list on NULL
      return NULL;
    }
  }

  // Do a pop. We use a reader count to prevent
  // a segment to be decommitted while a read is still pending,
  // and a tagged pointer to prevent A-B-A link corruption.
  // (this is called from `region.c:_mi_mem_free` for example)
  mi_atomic_increment_relaxed(&abandoned_readers);  // ensure no segment gets decommitted
  mi_tagged_segment_t next = 0;
  ts = mi_atomic_load_acquire(&abandoned);
  do {
    segment = mi_tagged_segment_ptr(ts);
    if (segment != NULL) {
      mi_segment_t* anext = mi_atomic_load_ptr_relaxed(mi_segment_t, &segment->abandoned_next);
      next = mi_tagged_segment(anext, ts); // note: reads the segment's `abandoned_next` field so should not be decommitted
    }
  } while (segment != NULL && !mi_atomic_cas_weak_acq_rel(&abandoned, &ts, next));
  mi_atomic_decrement_relaxed(&abandoned_readers);  // release reader lock
  if (segment != NULL) {
    mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, NULL);
    mi_atomic_decrement_relaxed(&abandoned_count);
  }
  return segment;
}

/* -----------------------------------------------------------
   Abandon segment/page
----------------------------------------------------------- */

static void mi_segment_abandon(mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_assert_internal(segment->used == segment->abandoned);
  mi_assert_internal(segment->used > 0);
  mi_assert_internal(mi_atomic_load_ptr_relaxed(mi_segment_t, &segment->abandoned_next) == NULL);
  mi_assert_internal(segment->abandoned_visits == 0);
  mi_assert_expensive(mi_segment_is_valid(segment,tld));
  
  // remove the free pages from the free page queues
  mi_slice_t* slice = &segment->slices[0];
  const mi_slice_t* end = mi_segment_slices_end(segment);
  while (slice < end) {
    mi_assert_internal(slice->slice_count > 0);
    mi_assert_internal(slice->slice_offset == 0);
    if (slice->xblock_size == 0) { // a free page
      mi_segment_span_remove_from_queue(slice,tld);
      slice->xblock_size = 0; // but keep it free
    }
    slice = slice + slice->slice_count;
  }

  // perform delayed decommits
  mi_segment_delayed_decommit(segment, mi_option_is_enabled(mi_option_abandoned_page_reset) /* force? */, tld->stats);    
  
  // all pages in the segment are abandoned; add it to the abandoned list
  _mi_stat_increase(&tld->stats->segments_abandoned, 1);
  mi_segments_track_size(-((long)mi_segment_size(segment)), tld);
  segment->thread_id = 0;
  mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, NULL);
  segment->abandoned_visits = 1;   // from 0 to 1 to signify it is abandoned
  mi_abandoned_push(segment);
}

void _mi_segment_page_abandon(mi_page_t* page, mi_segments_tld_t* tld) {
  mi_assert(page != NULL);
  mi_assert_internal(mi_page_thread_free_flag(page)==MI_NEVER_DELAYED_FREE);
  mi_assert_internal(mi_page_heap(page) == NULL);
  mi_segment_t* segment = _mi_page_segment(page);

  mi_assert_expensive(mi_segment_is_valid(segment,tld));
  segment->abandoned++;  

  _mi_stat_increase(&tld->stats->pages_abandoned, 1);
  mi_assert_internal(segment->abandoned <= segment->used);
  if (segment->used == segment->abandoned) {
    // all pages are abandoned, abandon the entire segment
    mi_segment_abandon(segment, tld);
  }
}

/* -----------------------------------------------------------
  Reclaim abandoned pages
----------------------------------------------------------- */

static mi_slice_t* mi_slices_start_iterate(mi_segment_t* segment, const mi_slice_t** end) {
  mi_slice_t* slice = &segment->slices[0];
  *end = mi_segment_slices_end(segment);
  mi_assert_internal(slice->slice_count>0 && slice->xblock_size>0); // segment allocated page
  slice = slice + slice->slice_count; // skip the first segment allocated page
  return slice;
}

// Possibly free pages and check if free space is available
static bool mi_segment_check_free(mi_segment_t* segment, size_t slices_needed, size_t block_size, mi_segments_tld_t* tld) 
{
  mi_assert_internal(block_size < MI_HUGE_BLOCK_SIZE);
  mi_assert_internal(mi_segment_is_abandoned(segment));
  bool has_page = false;
  
  // for all slices
  const mi_slice_t* end;
  mi_slice_t* slice = mi_slices_start_iterate(segment, &end);
  while (slice < end) {
    mi_assert_internal(slice->slice_count > 0);
    mi_assert_internal(slice->slice_offset == 0);
    if (mi_slice_is_used(slice)) { // used page
      // ensure used count is up to date and collect potential concurrent frees
      mi_page_t* const page = mi_slice_to_page(slice);
      _mi_page_free_collect(page, false);
      if (mi_page_all_free(page)) {
        // if this page is all free now, free it without adding to any queues (yet) 
        mi_assert_internal(page->next == NULL && page->prev==NULL);
        _mi_stat_decrease(&tld->stats->pages_abandoned, 1);
        segment->abandoned--;
        slice = mi_segment_page_clear(page, tld); // re-assign slice due to coalesce!
        mi_assert_internal(!mi_slice_is_used(slice));
        if (slice->slice_count >= slices_needed) {
          has_page = true;
        }
      }
      else {
        if (page->xblock_size == block_size && mi_page_has_any_available(page)) {
          // a page has available free blocks of the right size
          has_page = true;
        }
      }      
    }
    else {
      // empty span
      if (slice->slice_count >= slices_needed) {
        has_page = true;
      }
    }
    slice = slice + slice->slice_count;
  }
  return has_page;
}

// Reclaim an abandoned segment; returns NULL if the segment was freed
// set `right_page_reclaimed` to `true` if it reclaimed a page of the right `block_size` that was not full.
static mi_segment_t* mi_segment_reclaim(mi_segment_t* segment, mi_heap_t* heap, size_t requested_block_size, bool* right_page_reclaimed, mi_segments_tld_t* tld) {
  mi_assert_internal(mi_atomic_load_ptr_relaxed(mi_segment_t, &segment->abandoned_next) == NULL);
  mi_assert_expensive(mi_segment_is_valid(segment, tld));
  if (right_page_reclaimed != NULL) { *right_page_reclaimed = false; }

  segment->thread_id = _mi_thread_id();
  segment->abandoned_visits = 0;
  mi_segments_track_size((long)mi_segment_size(segment), tld);
  mi_assert_internal(segment->next == NULL);
  _mi_stat_decrease(&tld->stats->segments_abandoned, 1);
  
  // for all slices
  const mi_slice_t* end;
  mi_slice_t* slice = mi_slices_start_iterate(segment, &end);
  while (slice < end) {
    mi_assert_internal(slice->slice_count > 0);
    mi_assert_internal(slice->slice_offset == 0);
    if (mi_slice_is_used(slice)) {
      // in use: reclaim the page in our heap
      mi_page_t* page = mi_slice_to_page(slice);
      mi_assert_internal(!page->is_reset);
      mi_assert_internal(page->is_committed);
      mi_assert_internal(mi_page_thread_free_flag(page)==MI_NEVER_DELAYED_FREE);
      mi_assert_internal(mi_page_heap(page) == NULL);
      mi_assert_internal(page->next == NULL && page->prev==NULL);
      _mi_stat_decrease(&tld->stats->pages_abandoned, 1);
      segment->abandoned--;
      // set the heap again and allow delayed free again
      mi_page_set_heap(page, heap);
      _mi_page_use_delayed_free(page, MI_USE_DELAYED_FREE, true); // override never (after heap is set)
      _mi_page_free_collect(page, false); // ensure used count is up to date
      if (mi_page_all_free(page)) {
        // if everything free by now, free the page
        slice = mi_segment_page_clear(page, tld);   // set slice again due to coalesceing
      }
      else {
        // otherwise reclaim it into the heap
        _mi_page_reclaim(heap, page);
        if (requested_block_size == page->xblock_size && mi_page_has_any_available(page)) {
          if (right_page_reclaimed != NULL) { *right_page_reclaimed = true; }
        }
      }
    }
    else {
      // the span is free, add it to our page queues
      slice = mi_segment_span_free_coalesce(slice, tld); // set slice again due to coalesceing
    }
    mi_assert_internal(slice->slice_count>0 && slice->slice_offset==0);
    slice = slice + slice->slice_count;
  }

  mi_assert(segment->abandoned == 0);
  if (segment->used == 0) {  // due to page_clear
    mi_assert_internal(right_page_reclaimed == NULL || !(*right_page_reclaimed));
    mi_segment_free(segment, false, tld);
    return NULL;
  }
  else {
    return segment;
  }
}


void _mi_abandoned_reclaim_all(mi_heap_t* heap, mi_segments_tld_t* tld) {
  mi_segment_t* segment;
  while ((segment = mi_abandoned_pop()) != NULL) {
    mi_segment_reclaim(segment, heap, 0, NULL, tld);
  }
}

static mi_segment_t* mi_segment_try_reclaim(mi_heap_t* heap, size_t needed_slices, size_t block_size, bool* reclaimed, mi_segments_tld_t* tld)
{
  *reclaimed = false;
  mi_segment_t* segment;
  int max_tries = 8;     // limit the work to bound allocation times
  while ((max_tries-- > 0) && ((segment = mi_abandoned_pop()) != NULL)) {
    segment->abandoned_visits++;
    bool has_page = mi_segment_check_free(segment,needed_slices,block_size,tld); // try to free up pages (due to concurrent frees)
    if (segment->used == 0) {
      // free the segment (by forced reclaim) to make it available to other threads.
      // note1: we prefer to free a segment as that might lead to reclaiming another
      // segment that is still partially used.
      // note2: we could in principle optimize this by skipping reclaim and directly
      // freeing but that would violate some invariants temporarily)
      mi_segment_reclaim(segment, heap, 0, NULL, tld);
    }
    else if (has_page) {
      // found a large enough free span, or a page of the right block_size with free space 
      // we return the result of reclaim (which is usually `segment`) as it might free
      // the segment due to concurrent frees (in which case `NULL` is returned).
      return mi_segment_reclaim(segment, heap, block_size, reclaimed, tld);
    }
    else if (segment->abandoned_visits > 3) {  
      // always reclaim on 3rd visit to limit the abandoned queue length.
      mi_segment_reclaim(segment, heap, 0, NULL, tld);
    }
    else {
      // otherwise, push on the visited list so it gets not looked at too quickly again
      mi_segment_delayed_decommit(segment, false, tld->stats); // decommit if needed
      mi_abandoned_visited_push(segment);
    }
  }
  return NULL;
}


/* -----------------------------------------------------------
   Reclaim or allocate
----------------------------------------------------------- */

static mi_segment_t* mi_segment_reclaim_or_alloc(mi_heap_t* heap, size_t needed_slices, size_t block_size, mi_segments_tld_t* tld, mi_os_tld_t* os_tld) 
{
  mi_assert_internal(block_size < MI_HUGE_BLOCK_SIZE);
  mi_assert_internal(block_size <= MI_LARGE_OBJ_SIZE_MAX);
  // 1. try to get a segment from our cache
  mi_segment_t* segment = mi_segment_cache_pop(MI_SEGMENT_SIZE, tld);
  if (segment != NULL) {
    mi_segment_init(segment, 0, tld, os_tld, NULL);
    return segment;
  }
  // 2. try to reclaim an abandoned segment
  bool reclaimed;
  segment = mi_segment_try_reclaim(heap, needed_slices, block_size, &reclaimed, tld);
  if (reclaimed) {
    // reclaimed the right page right into the heap
    mi_assert_internal(segment != NULL);
    return NULL; // pretend out-of-memory as the page will be in the page queue of the heap with available blocks
  }
  else if (segment != NULL) {
    // reclaimed a segment with a large enough empty span in it
    return segment;
  }
  // 3. otherwise allocate a fresh segment
  return mi_segment_alloc(0, tld, os_tld, NULL);  
}


/* -----------------------------------------------------------
   Page allocation
----------------------------------------------------------- */

static mi_page_t* mi_segments_page_alloc(mi_heap_t* heap, mi_page_kind_t page_kind, size_t required, size_t block_size, mi_segments_tld_t* tld, mi_os_tld_t* os_tld)
{
  mi_assert_internal(required <= MI_LARGE_OBJ_SIZE_MAX && page_kind <= MI_PAGE_LARGE);

  // find a free page
  size_t page_size = _mi_align_up(required, (required > MI_MEDIUM_PAGE_SIZE ? MI_MEDIUM_PAGE_SIZE : MI_SEGMENT_SLICE_SIZE));
  size_t slices_needed = page_size / MI_SEGMENT_SLICE_SIZE;
  mi_assert_internal(slices_needed * MI_SEGMENT_SLICE_SIZE == page_size);
  mi_page_t* page = mi_segments_page_find_and_allocate(slices_needed, tld); //(required <= MI_SMALL_SIZE_MAX ? 0 : slices_needed), tld);
  if (page==NULL) {
    // no free page, allocate a new segment and try again
    if (mi_segment_reclaim_or_alloc(heap, slices_needed, block_size, tld, os_tld) == NULL) {
      // OOM or reclaimed a good page in the heap
      return NULL;  
    }
    else {
      // otherwise try again
      return mi_segments_page_alloc(heap, page_kind, required, block_size, tld, os_tld);
    }
  }
  mi_assert_internal(page != NULL && page->slice_count*MI_SEGMENT_SLICE_SIZE == page_size);
  mi_assert_internal(_mi_ptr_segment(page)->thread_id == _mi_thread_id());
  mi_segment_delayed_decommit(_mi_ptr_segment(page), false, tld->stats);
  return page;
}



/* -----------------------------------------------------------
   Huge page allocation
----------------------------------------------------------- */

static mi_page_t* mi_segment_huge_page_alloc(size_t size, mi_segments_tld_t* tld, mi_os_tld_t* os_tld)
{
  mi_page_t* page = NULL;
  mi_segment_t* segment = mi_segment_alloc(size,tld,os_tld,&page);
  if (segment == NULL || page==NULL) return NULL;
  mi_assert_internal(segment->used==1);
  mi_assert_internal(mi_page_block_size(page) >= size);  
  segment->thread_id = 0; // huge segments are immediately abandoned
  return page;
}

// free huge block from another thread
void _mi_segment_huge_page_free(mi_segment_t* segment, mi_page_t* page, mi_block_t* block) {
  // huge page segments are always abandoned and can be freed immediately by any thread
  mi_assert_internal(segment->kind==MI_SEGMENT_HUGE);
  mi_assert_internal(segment == _mi_page_segment(page));
  mi_assert_internal(mi_atomic_load_relaxed(&segment->thread_id)==0);

  // claim it and free
  mi_heap_t* heap = mi_heap_get_default(); // issue #221; don't use the internal get_default_heap as we need to ensure the thread is initialized.
  // paranoia: if this is the last reference, the cas should always succeed
  uintptr_t expected_tid = 0;
  if (mi_atomic_cas_strong_acq_rel(&segment->thread_id, &expected_tid, heap->thread_id)) {
    mi_block_set_next(page, block, page->free);
    page->free = block;
    page->used--;
    page->is_zero = false;
    mi_assert(page->used == 0);
    mi_tld_t* tld = heap->tld;
    _mi_segment_page_free(page, true, &tld->segments);
  }
#if (MI_DEBUG!=0)
  else {
    mi_assert_internal(false);
  }
#endif
}

/* -----------------------------------------------------------
   Page allocation and free
----------------------------------------------------------- */
mi_page_t* _mi_segment_page_alloc(mi_heap_t* heap, size_t block_size, mi_segments_tld_t* tld, mi_os_tld_t* os_tld) {
  mi_page_t* page;
  if (block_size <= MI_SMALL_OBJ_SIZE_MAX) {
    page = mi_segments_page_alloc(heap,MI_PAGE_SMALL,block_size,block_size,tld,os_tld);
  }
  else if (block_size <= MI_MEDIUM_OBJ_SIZE_MAX) {
    page = mi_segments_page_alloc(heap,MI_PAGE_MEDIUM,MI_MEDIUM_PAGE_SIZE,block_size,tld, os_tld);
  }
  else if (block_size <= MI_LARGE_OBJ_SIZE_MAX) {
    page = mi_segments_page_alloc(heap,MI_PAGE_LARGE,block_size,block_size,tld, os_tld);
  }
  else {
    page = mi_segment_huge_page_alloc(block_size,tld,os_tld);
  }
  mi_assert_expensive(page == NULL || mi_segment_is_valid(_mi_page_segment(page),tld));
  return page;
}


