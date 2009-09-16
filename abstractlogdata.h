#ifndef ABSTRACTLOGDATA_H
#define ABSTRACTLOGDATA_H

#include <QByteArray>
#include <QString>

class AbstractLogData {
    public:
        AbstractLogData();

        QString getLineString(int line) const;
        int getNbLine() const;
        int getMaxLength() const;

    protected:
        virtual QString doGetLineString(int line) const = 0;
        virtual int doGetNbLine() const = 0;

        ~AbstractLogData() {};      // Don't allow polymorphic destruction

        int maxLength;
};

#endif
