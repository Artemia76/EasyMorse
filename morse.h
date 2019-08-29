#ifndef MORSE_H
#define MORSE_H

#include <QObject>
#include <QMap>

#include "generateur.h"

class morse : public QObject
{
    Q_OBJECT
public:
    explicit morse(QObject *parent = nullptr );
    enum Code
    {
        American,
        Continental,
        International
    };
    Q_ENUM(Code)
    void initMorseMap (Code pCode=International);
    void code(const QString& pMessage);
    Generateur* data();
private:
    QMap <QString, QString> m_MorseMapping;
    qint64 m_dotDuration;
    QScopedPointer<Generateur> m_generator;
signals:

public slots:
};

#endif // MORSE_H
