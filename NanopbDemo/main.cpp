#include <conio.h> // _getch()
#include <map>
#include <stdio.h>
#include <string>
#include <vector>

#include <pb_decode.h>
#include <pb_encode.h>

#include "demo.pb.h"

using namespace std;

typedef map<int, string> MsgTypes_t;

typedef struct Field {
    int32_t type;            // 字段类型
    string column;           // 字段名称
    vector<uint8_t> content; // 字段内容
} Field_t;

typedef vector<Field_t> Row_t;
typedef vector<Row_t> Rows_t;

static void print_buffer(uint8_t *buffer, size_t len, const char *prefix, bool newline)
{
    if (prefix != NULL) {
        printf("%s[%d]:\n", prefix, len);
    }

    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
    }
    if (newline)
        printf("\n");
}

static void print_content(vector<uint8_t> c, int type)
{
    switch (type) {
        case 1:
            printf("%d", c.front());
            break;
        case 2: {
            // 强制转换，忽略正确性
            uint8_t *p = c.data();
            float f    = *(float *)p;
            printf("%f", f);
            break;
        }
        case 3: {
            // TODO: 解码
            string str(c.begin(), c.end());
            printf("%s", str.c_str());
            break;
        }
        case 4: {
            uint8_t *p = c.data();
            for (size_t i = 0; i < c.size(); i++) {
                printf("%02X ", p[i]);
            }
            break;
        }
        case 5:
            printf("NULL");
            break;
        default:
            break;
    }
}

static bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    const char *str = (const char *)*arg;

    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    return pb_encode_string(stream, (uint8_t *)str, strlen(str));
}

static bool decode_string(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    string *str = static_cast<string *>(*arg);
    size_t len  = stream->bytes_left;
    str->resize(len);
    if (!pb_read(stream, (uint8_t *)str->data(), len)) {
        return false;
    }
    return true;
}

static bool encode_bytes(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    vector<uint8_t> *v = (vector<uint8_t> *)*arg;

    if (!pb_encode_tag_for_field(stream, field)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(stream));
        return false;
    }

    return pb_encode_string(stream, (uint8_t *)v->data(), v->size());
}

static bool decode_bytes(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    vector<uint8_t> *bytes = static_cast<vector<uint8_t> *>(*arg);
    size_t len             = stream->bytes_left;
    bytes->resize(len);
    if (!pb_read(stream, (uint8_t *)bytes->data(), len)) {
        return false;
    }
    return true;
}

static bool encode_types(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    MsgTypes_t *m               = (MsgTypes_t *)*arg;
    MsgTypes_TypesEntry message = MsgTypes_TypesEntry_init_default;

    for (auto it = m->begin(); it != m->end(); it++) {
        message.key                = it->first;
        message.value.funcs.encode = &encode_string;
        message.value.arg          = (void *)it->second.c_str();

        if (!pb_encode_tag_for_field(stream, field)) {
            return false;
        }

        if (!pb_encode_submessage(stream, MsgTypes_TypesEntry_fields, &message)) {
            return false;
        }
    }

    return true;
}

static bool decode_types(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    map<int, string> *m = (map<int, string> *)*arg;
    string str;
    MsgTypes_TypesEntry message = MsgTypes_TypesEntry_init_default;
    message.value.funcs.decode  = &decode_string;
    message.value.arg           = &str;

    if (!pb_decode(stream, MsgTypes_TypesEntry_fields, &message)) {
        return false;
    }

    (*m)[message.key] = str;

    return true;
}

static bool cb_msg_types(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    Response *rsp = (Response *)field->message;

    if (field->tag == Response_types_tag) {
        MsgTypes *msg           = (MsgTypes *)field->pData;
        msg->types.funcs.decode = decode_types;
        msg->types.arg          = *arg;
    }

    return true;
}

static bool encode_req(vector<uint8_t> *buffer, Request req)
{
    size_t len;
    if (!pb_get_encoded_size(&len, Request_fields, &req)) {
        return false;
    }
    buffer->resize(len);

    pb_ostream_t stream = pb_ostream_from_buffer(&buffer->front(), len);
    if (!pb_encode(&stream, Request_fields, &req)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    return true;
}

static bool decode_req(uint8_t *buffer, size_t len, Request *req)
{
    pb_istream_t stream = pb_istream_from_buffer(buffer, len);

    if (!pb_decode(&stream, Request_fields, req)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    return true;
}

static bool encode_dbfield(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    Row_t *v        = (Row_t *)*arg;
    DbField dbfield = DbField_init_default;

    for (auto it = v->begin(); it != v->end(); it++) {
        dbfield.type = (*it).type;

        dbfield.column.arg          = (void *)(*it).column.c_str();
        dbfield.column.funcs.encode = &encode_string;

        dbfield.content.arg          = (void *)&(*it).content;
        dbfield.content.funcs.encode = &encode_bytes;

        if (!pb_encode_tag_for_field(stream, field)) {
            printf("Encoding failed: %s\n", PB_GET_ERROR(stream));
            return false;
        }

        if (!pb_encode_submessage(stream, DbField_fields, &dbfield)) {
            printf("Encoding failed: %s\n", PB_GET_ERROR(stream));
            return false;
        }
    }

    return true;
}

static bool decode_dbfield(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    Field_t data    = {};
    DbField dbfield = DbField_init_default;

    dbfield.column.funcs.decode = &decode_string;
    dbfield.column.arg          = (void *)&(data.column);

    dbfield.content.funcs.decode = &decode_bytes;
    dbfield.content.arg          = (void *)&(data.content);

    if (!pb_decode(stream, DbField_fields, &dbfield)) {
        printf("Decoding DbField_fields failed: %s\n", PB_GET_ERROR(stream));
        pb_release(DbField_fields, &dbfield);
        return false;
    }
    data.type = dbfield.type;
    printf("%d\t%s\t", data.type, data.column.c_str());
    print_content(data.content, data.type);
    printf("\n");
    return true;
}

static bool encode_dbrow(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    Rows_t *v   = (Rows_t *)*arg;
    DbRow dbrow = DbRow_init_default;

    for (auto it = v->begin(); it != v->end(); it++) {
        dbrow.fields.arg          = (void *)&(*it);
        dbrow.fields.funcs.encode = &encode_dbfield;

        if (!pb_encode_tag_for_field(stream, field)) {
            printf("Encoding failed: %s\n", PB_GET_ERROR(stream));
            return false;
        }

        if (!pb_encode_submessage(stream, DbRow_fields, &dbrow)) {
            printf("Encoding failed: %s\n", PB_GET_ERROR(stream));
            return false;
        }
    }

    return true;
}

static bool decode_dbrow(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    DbRow dbrow               = DbRow_init_default;
    dbrow.fields.funcs.decode = &decode_dbfield;
    dbrow.fields.arg          = *arg;
    if (!pb_decode(stream, DbRow_fields, &dbrow)) {
        printf("Decoding DbRow_fields failed: %s\n", PB_GET_ERROR(stream));
        pb_release(DbRow_fields, &dbrow);
        return false;
    }
    return true;
}

static bool encode_dbrows(Rows_t *rows, uint8_t *out, size_t *len)
{
    Response rsp  = Response_init_default;
    rsp.func      = Functions_FUNC_EXEC_DB_QUERY;
    rsp.which_msg = Response_rows_tag;

    rsp.msg.rows.rows.arg          = rows;
    rsp.msg.rows.rows.funcs.encode = encode_dbrow;

    pb_ostream_t stream = pb_ostream_from_buffer(out, *len);
    if (!pb_encode(&stream, Response_fields, &rsp)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    *len = stream.bytes_written;

    return true;
}

static bool decode_dbrows(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    DbRows dbrows            = DbRows_init_default;
    dbrows.rows.funcs.decode = &decode_dbrow;
    dbrows.rows.arg          = *arg;

    if (!pb_decode(stream, DbRows_fields, &dbrows)) {
        printf("Decoding DbRows_fields failed: %s\n", PB_GET_ERROR(stream));
        pb_release(DbRows_fields, &dbrows);
        return false;
    }

    return true;
}

static bool cb_dbrows(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    Response *rsp = (Response *)field->message;

    if (field->tag == Response_rows_tag) {
        DbRows *dbrows            = (DbRows *)field->pData;
        dbrows->rows.funcs.decode = decode_dbrow;
        dbrows->rows.arg          = *arg;
    }

    return true;
}

static void test_is_login()
{
    vector<uint8_t> buffer;
    Request enc   = Request_init_default;
    enc.func      = Functions_FUNC_IS_LOGIN;
    enc.which_msg = Request_empty_tag;

    encode_req(&buffer, enc);
    print_buffer(&buffer.front(), buffer.size(), "Encoded message", true);

    Request dec = Request_init_default;
    decode_req(&buffer.front(), buffer.size(), &dec);
    printf("Decoded message:\nFunc: %02x\n", dec.func);
    pb_release(Request_fields, &dec);
    printf("---------------------------------------\n");
}

static void test_get_self_wxid()
{
    vector<uint8_t> buffer;
    Request enc   = Request_init_default;
    enc.func      = Functions_FUNC_GET_SELF_WXID;
    enc.which_msg = Request_empty_tag;

    encode_req(&buffer, enc);
    print_buffer(&buffer.front(), buffer.size(), "Encoded message", true);

    Request dec = Request_init_default;
    decode_req(&buffer.front(), buffer.size(), &dec);
    printf("Decoded message:\nFunc: %02x\n", dec.func);
    pb_release(Request_fields, &dec);
    printf("---------------------------------------\n");
}

static void test_send_txt(char *msg, char *receiver, char *aters)
{
    vector<uint8_t> buffer;

    Request enc   = Request_init_default;
    enc.func      = Functions_FUNC_SEND_TXT;
    enc.which_msg = Request_txt_tag;
    enc.msg.txt   = { msg, receiver, aters };

    encode_req(&buffer, enc);
    print_buffer(&buffer.front(), buffer.size(), "Encoded message", true);

    Request dec = Request_init_default;
    decode_req(&buffer.front(), buffer.size(), &dec);
    printf("Decoded message:\nFunc: %02x, ", dec.func);
    printf("\nmsg: %s\nreceiver: %s\naters: %s\n", dec.msg.txt.msg, dec.msg.txt.receiver, dec.msg.txt.aters);
    pb_release(Request_fields, &dec);
    printf("---------------------------------------\n");
}

static void test_get_msg_types()
{
    size_t len = 1024;
    uint8_t buffer[1024];
    MsgTypes_t types = { { 0x01, "文字" },
                         { 0x03, "图片" },
                         { 0x22, "语音" },
                         { 0x25, "好友确认" },
                         { 0x28, "POSSIBLEFRIEND_MSG" },
                         { 0x2A, "名片" },
                         { 0x2B, "视频" },
                         { 0x2F, "石头剪刀布 | 表情图片" },
                         { 0x30, "位置" },
                         { 0x31, "共享实时位置、文件、转账、链接" },
                         { 0x32, "VOIPMSG" },
                         { 0x33, "微信初始化" },
                         { 0x34, "VOIPNOTIFY" },
                         { 0x35, "VOIPINVITE" },
                         { 0x3E, "小视频" },
                         { 0x270F, "SYSNOTICE" },
                         { 0x2710, "红包、系统消息" },
                         { 0x2712, "撤回消息" } };

    Response enc  = Response_init_default;
    enc.func      = Functions_FUNC_GET_MSG_TYPES;
    enc.which_msg = Response_types_tag;

    enc.msg.types.types.funcs.encode = encode_types;
    enc.msg.types.types.arg          = &types;

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, len);
    if (!pb_encode(&stream, Response_fields, &enc)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return;
    }
    len = stream.bytes_written;
    print_buffer(buffer, len, "Encoded message", true);

    MsgTypes_t out;
    Response dec            = Response_init_default;
    pb_istream_t ostream    = pb_istream_from_buffer(buffer, len);
    dec.cb_msg.funcs.decode = cb_msg_types;
    dec.cb_msg.arg          = &out;
    if (!pb_decode(&ostream, Response_fields, &dec)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return;
    }
    printf("Decoded message:\nFunc: %02x, %02X\n", dec.func, dec.which_msg);

    for (auto it = out.begin(); it != out.end(); it++) {
        printf("key: %d, value: %s\r\n", it->first, it->second.c_str());
    }
    pb_release(Response_fields, &dec);
    printf("---------------------------------------\n");
}

static void test_query_sql()
{
    size_t size = 1024;
    vector<uint8_t> buffer;

    Rows_t enc = {};
    for (int r = 0; r < 5; r++) {
        Row_t row;
        for (int f = 1; f < 6; f++) {
            Field_t field; // 随便赋值，忽略正确性
            field.type    = f;
            field.column  = "Column" + to_string(r) + to_string(f);
            field.content = { (uint8_t)('A' + r + f), 'C', 'o', 'n', 't', 'e', 'n', 't' };
            row.push_back(field);
        }
        enc.push_back(row);
    }

    buffer.resize(size);
    encode_dbrows(&enc, buffer.data(), &size);
    print_buffer(&buffer.front(), size, "Encoded message", true);

    pb_istream_t stream = pb_istream_from_buffer(&buffer.front(), size);

    Rows_t dec; // 此变量可传入解码方法中，存储解码结果
    Response rsp            = Response_init_default;
    rsp.cb_msg.funcs.decode = cb_dbrows;
    rsp.cb_msg.arg          = &dec;

    if (!pb_decode(&stream, Response_fields, &rsp)) {
        printf("Decoding Response_fields failed: %s\n", PB_GET_ERROR(&stream));
        pb_release(Response_fields, &rsp);
        return;
    }
}

int main()
{
    test_is_login();
    test_get_self_wxid();
    test_send_txt((char *)"This is message", (char *)"TO CHUCK", (char *)"@all");
    test_get_msg_types();
    test_query_sql();

    return _getch();
}
