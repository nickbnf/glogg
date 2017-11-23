#include "encodingdetector.h"
#include <QTextCodec>
#include <QMutexLocker>

#include "uchardet.h"
#include "log.h"

namespace {

class UchardetHolder {
  public:
    UchardetHolder() : ud_{ uchardet_new() } {}
    ~UchardetHolder() { uchardet_delete( ud_ ); }

    UchardetHolder(const UchardetHolder&) = delete;
    UchardetHolder& operator=(const UchardetHolder&) = delete;

    UchardetHolder(UchardetHolder&& other) = delete;
    UchardetHolder& operator =(UchardetHolder&& other) = delete;

    int handle_data( const char * data, size_t len )
    {
        return uchardet_handle_data( ud_ , data, len );
    }

    void data_end() { uchardet_data_end( ud_ ); }

    const char* get_charset() { return uchardet_get_charset( ud_ ); }

private:
    uchardet_t ud_;
};

}

QTextCodec* EncodingDetector::detectEncoding( const QByteArray& block ) const
{
    QMutexLocker lock(&mutex_);

    UchardetHolder ud;

    auto rc = ud.handle_data( block.data(), block.size() );
    if ( rc == 0 ) {
       ud.data_end();
    }

    QTextCodec* uchardetCodec = nullptr;
    if ( rc == 0 ) {
        auto uchardetGuess = ud.get_charset();
        LOG(logINFO) << "Uchardet encoding guess " << uchardetGuess;
        uchardetCodec =  QTextCodec::codecForName( uchardetGuess );
        if ( uchardetCodec ) {
            LOG(logINFO) << "Uchardet codec selected " << uchardetCodec->name().constData();
        } else {
            LOG(logWARNING) << "Uchardet codec not found for guess " << uchardetGuess;
        }
    }

    auto encodingGuess = uchardetCodec ? QTextCodec::codecForUtfText( block, uchardetCodec )
                                       : QTextCodec::codecForUtfText( block );

    LOG(logINFO) << "Final encoding guess " << encodingGuess->name().constData();

    return encodingGuess;
}
