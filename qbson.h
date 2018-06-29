#ifndef BSON_H
#define BSON_H

#include <QtDebug>
#include <QVariant>
#include <QDateTime>
#include <QException>

#include <bsoncxx/document/value.hpp>

class BSONexception : public QException {

public:
    BSONexception(const QString & data) noexcept {i_data = data;}
    ~BSONexception() noexcept {}

    void raise() const { throw *this; }
    BSONexception *clone() const { return new BSONexception(*this); }
    const QString & data() const { return i_data; }

private:
    QString i_data;
};

namespace BSON {

bsoncxx::document::value toBson(const QVariantMap &obj, bool &ok) noexcept;
bsoncxx::document::value toBson(const QVariantMap &obj) throw (BSONexception);

QVariantMap fromBson(const bsoncxx::document::value &bson, bool &ok) noexcept;
QVariantMap fromBson(const bsoncxx::document::value &bson) throw (BSONexception);
QVariantMap fromBson(const bsoncxx::document::view &bson, bool &ok) noexcept;
QVariantMap fromBson(const bsoncxx::document::view &bson) throw (BSONexception);

QVariant fromBsonValue(const bsoncxx::types::value & value, bool &ok) noexcept;
QVariant fromBsonValue(const bsoncxx::types::value & value) throw (BSONexception);

void init();

}

struct BSONbinary
{
    enum Type {
        Unknown = 0,
        Function,
        MD5
//        User
    } type = {Unknown};

    QByteArray data;
};
Q_DECLARE_METATYPE(BSONbinary)
inline bool operator==(const BSONbinary& rhs, const BSONbinary& lhs);
QDebug operator<<(QDebug debug, const BSONbinary &c);
QDataStream &operator<<(QDataStream &out, const BSONbinary &value);
QDataStream &operator>>(QDataStream &in, BSONbinary &value);

struct BSONoid
{
    QByteArray data;
    QDateTime time;

    BSONoid() {}

    BSONoid(QByteArray hex) {
        data = QByteArray::fromHex(hex);
    }

    QString toHex() const {
        return data.toHex();
    }
};
Q_DECLARE_METATYPE(BSONoid)
inline bool operator==(const BSONoid& lhs, const BSONoid& rhs);
QDebug operator<<(QDebug debug, const BSONoid &c);
QDataStream &operator<<(QDataStream &out, const BSONoid &value);
QDataStream &operator>>(QDataStream &in, BSONoid &value);

struct BSONregexp
{
    QString regexp;
    QString options;
};
Q_DECLARE_METATYPE(BSONregexp)
inline bool operator==(const BSONregexp& lhs, const BSONregexp& rhs);
QDebug operator<<(QDebug debug, const BSONregexp &c);
QDataStream &operator<<(QDataStream &out, const BSONregexp &value);
QDataStream &operator>>(QDataStream &in, BSONregexp &value);

struct BSONcode
{
    QString code;


};
Q_DECLARE_METATYPE(BSONcode)
inline bool operator==(const BSONcode& lhs, const BSONcode& rhs);
QDebug operator<<(QDebug debug, const BSONcode &c);
QDataStream &operator<<(QDataStream &out, const BSONcode &value);
QDataStream &operator>>(QDataStream &in, BSONcode &value);

struct BSONcodeWscope
{
    QString code;
    QVariantMap scope;
};
Q_DECLARE_METATYPE(BSONcodeWscope)
inline bool operator==(const BSONcodeWscope& lhs, const BSONcodeWscope& rhs);
QDebug operator<<(QDebug debug, const BSONcodeWscope &c);
QDataStream &operator<<(QDataStream &out, const BSONcodeWscope &value);
QDataStream &operator>>(QDataStream &in, BSONcodeWscope &value);

struct BSONmaxkey
{};
Q_DECLARE_METATYPE(BSONmaxkey)
inline bool operator==(const BSONmaxkey&, const BSONmaxkey&);
QDebug operator<<(QDebug debug, const BSONmaxkey &);
QDataStream &operator<<(QDataStream &out, const BSONmaxkey &value);
QDataStream &operator>>(QDataStream &in, BSONmaxkey &value);

struct BSONminkey
{};
Q_DECLARE_METATYPE(BSONminkey)
inline bool operator==(const BSONminkey&, const BSONminkey&);
QDebug operator<<(QDebug debug, const BSONminkey &);
QDataStream &operator<<(QDataStream &out, const BSONminkey &value);
QDataStream &operator>>(QDataStream &in, BSONminkey &value);


#endif // BSON_H
