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

    LOG(logDEBUG2) << "Get block " << elementSize_
                   << " pool " << pool_.size()
                   << " alloc " << allocationSize_
                   << " blocks " << blockIndex_.size();

    if ( allocationSize_ + requiredSize >= pool_.size() ) {
        increasePool( pool_ );
    }

    blockIndex_.push_back( allocationSize_ );
    allocationSize_ += requiredSize;

    return pool_.data() + blockIndex_.back();
}

uint8_t* BlockPoolBase::resizeLastBlock( size_t newSize )
{
    const auto alignedNewSize = getAlignedSize( newSize, alignment_ );
    const auto currentBlockSize = lastBlockSize();

    LOG(logDEBUG2) << "Resizing block "
                    << " from " << currentBlockSize
                    << " to " << newSize
                    << " aligned " << alignedNewSize
                    << " alloc " << allocationSize_;

    if ( alignedNewSize <= currentBlockSize ) {
        allocationSize_ -= ( currentBlockSize - alignedNewSize );
    }
    else {
        const auto delta = alignedNewSize - currentBlockSize;
        LOG(logDEBUG2) << "Increasing last block size by " << delta;

        if ( allocationSize_ + delta >= pool_.size() ) {
            increasePool( pool_ );
        }
        allocationSize_ += delta;
    }

    LOG(logDEBUG2) << "Resized block, alloc " << allocationSize_;

    return pool_.data() + blockIndex_.back();
}

size_t BlockPoolBase::lastBlockSize() const
{
    if ( blockIndex_.empty() ) {
        return 0;
    }

    return allocationSize_ - blockIndex_.back();
}

void BlockPoolBase::freeLastBlock()
{
    if ( blockIndex_.empty() ) {
        return;
    }

    const auto freeSize = lastBlockSize();
    LOG(logDEBUG2) << "Free block " << freeSize;

    allocationSize_ = blockIndex_.back();

    LOG(logDEBUG2) << "Free block, alloc " << allocationSize_;

    blockIndex_.pop_back();
}
