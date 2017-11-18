#include "encodingdetector.h"
#include <QTextCodec>
#include <QMutexLocker>


#include "uchardet.h"
#include "log.h"

namespace {

class UchardetHolder {
  public:
    UchardetHolder() : ud_{uchardet_new()} {}
    ~UchardetHolder() { uchardet_delete(ud_); }

     operator uchardet_t() { return ud_; }

private:
    uchardet_t ud_;
};

}

QTextCodec* EncodingDetector::detectEncoding( const QByteArray& block ) const
{
    QMutexLocker lock(&mutex_);

    UchardetHolder ud;

    auto rc = uchardet_handle_data( ud, block.data(), block.size() );
    if ( rc == 0 ) {
       uchardet_data_end( ud );
    }

    QTextCodec* uchardetCodec = nullptr;
    if ( rc == 0 ) {
        auto uchardetGuess = uchardet_get_charset( ud );
        LOG(logINFO) << "Uchardet encoding guess " << uchardetGuess;
        uchardetCodec =  QTextCodec::codecForName( uchardetGuess );
        if ( uchardetCodec ) {
            LOG(logINFO) << "Uchardet codec selected " << uchardetCodec->name().data();
        } else {
            LOG(logWARNING) << "Uchardet codec not found for guess " << uchardetGuess;
        }
    }

    auto encodingGuess = uchardetCodec ? QTextCodec::codecForUtfText( block, uchardetCodec )
                                       : QTextCodec::codecForUtfText( block );

    LOG(logINFO) << "Final encoding guess " << encodingGuess->name().data();

    return encodingGuess;
}
