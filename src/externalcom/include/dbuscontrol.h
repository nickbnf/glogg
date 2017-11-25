#include <QObject>
#include <QString>

class DBusControl : public QObject {
  Q_OBJECT

  public slots:
    QString version(void) { return "1.0.0"; }
};
