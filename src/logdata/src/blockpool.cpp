#include "blockpool.h"

#include "log.h"

namespace {

size_t getElementSizeWithHeader( std::size_t elementSize )
{
    return elementSize + sizeof( uint16_t );
}

size_t getAlignedSize( size_t required_size, size_t alignement )
{
    return required_size + required_size % alignement;
}

size_t getBlockStorageSize( size_t elementsCount, std::size_t elementSize, size_t alignement )
{
    return getAlignedSize( elementSize + elementsCount * getElementSizeWithHeader( elementSize ), alignement );
}

template<typename PoolType>
void increasePool(PoolType& pool)
{
    const auto oldSize = pool.size();
    pool.resize( oldSize + (oldSize >> 1 ) );
}

}

BlockPoolBase::BlockPoolBase( size_t elementSize, size_t alignment )
    : pool_(1024*1024)
    , elementSize_ {elementSize}
    , alignment_ {alignment}
    , allocationSize_{}
    , lastBlockSize_{}
{
    blockIndex_.reserve( 10000 );
}

BlockPoolBase::BlockPoolBase( BlockPoolBase&& other )
{
    *this = std::move( other );
}

BlockPoolBase& BlockPoolBase::operator=( BlockPoolBase&& other )
{
    pool_ = std::move( other.pool_ );

    elementSize_ = other.elementSize_;
    alignment_ = other.alignment_;

    allocationSize_ = other.allocationSize_;
    lastBlockSize_ = other.lastBlockSize_;

    blockIndex_ = std::move( other.blockIndex_ );

    return *this;
}

uint8_t* BlockPoolBase::operator[](size_t index)
{
    return pool_.data() + blockIndex_.at( index );
}

const uint8_t* BlockPoolBase::operator[](size_t index) const
{
    return pool_.data() + blockIndex_.at( index );
}

size_t BlockPoolBase::getElementSize() const
{
    return elementSize_;
}

size_t BlockPoolBase::getPaddedElementSize() const
{
    return getElementSizeWithHeader( elementSize_ );
}

uint32_t BlockPoolBase::currentBlock() const
{
    return static_cast<uint32_t>( blockIndex_.size() - 1 );
}

uint8_t* BlockPoolBase::getBlock( size_t elementsCount )
{
    const auto requiredSize = getBlockStorageSize( elementsCount, elementSize_, alignment_ );

    LOG(logINFO) << "Get block " << elementSize_
                   << " pool " << pool_.size()
                   << " alloc " << allocationSize_
                   << " blocks " << blockIndex_.size();

    if ( allocationSize_ + requiredSize >= pool_.size() ) {
        increasePool( pool_ );
    }

    blockIndex_.push_back( allocationSize_ );
    allocationSize_ += requiredSize;
    lastBlockSize_ = requiredSize;

    return pool_.data() + blockIndex_.back();
}

uint8_t* BlockPoolBase::resizeLastBlock( size_t newSize )
{
    const auto alignedNewSize = getAlignedSize( newSize, alignment_ );

    LOG(logINFO) << "Resizing block "
                    << " from " << lastBlockSize_
                    << " to " << newSize
                    << " aligned " << alignedNewSize
                    << " alloc " << allocationSize_;

    if ( alignedNewSize < lastBlockSize_ ) {
        allocationSize_ -= ( lastBlockSize_ - alignedNewSize );
    }
    else {
        const auto delta = alignedNewSize - lastBlockSize_;
        LOG(logINFO) << "Increasing last block size by " << delta;

        if ( allocationSize_ + delta >= pool_.size() ) {
            increasePool( pool_ );
        }
        allocationSize_ += delta;
    }

    lastBlockSize_ = alignedNewSize;

    LOG(logINFO) << "Resized block, alloc " << allocationSize_;

    return pool_.data() + blockIndex_.back();
}

void BlockPoolBase::freeLastBlock()
{
    LOG(logDEBUG2) << "Free block " << lastBlockSize_;

    if (allocationSize_ >= lastBlockSize_)
    {
        allocationSize_ -= lastBlockSize_;
    }

    LOG(logINFO) << "Free block, alloc " << allocationSize_;

    blockIndex_.pop_back();
}
