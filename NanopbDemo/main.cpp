#include <conio.h> // _getch()
#include <map>
#include <stdio.h>
#include <string>
#include <vector>

#include <pb_decode.h>
#include <pb_encode.h>

#include "demo.pb.h"

typedef std::map<int, std::string> MsgTypes_t;

void print_buffer(uint8_t *buffer, size_t len)
{
    printf("Encoded message[%d]:\n", len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    const char *str = (const char *)*arg;

    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    return pb_encode_string(stream, (uint8_t *)str, strlen(str));
}

bool decode_string(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    std::string *str = static_cast<std::string *>(*arg);
    size_t len       = stream->bytes_left;
    str->resize(len);
    if (!pb_read(stream, (uint8_t *)str->data(), len)) {
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

bool decode_types(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    std::map<int, std::string> *m = (std::map<int, std::string> *)*arg;
    std::string str;
    MsgTypes_TypesEntry message = MsgTypes_TypesEntry_init_default;
    message.value.funcs.decode  = &decode_string;
    message.value.arg           = &str;

    if (!pb_decode(stream, MsgTypes_TypesEntry_fields, &message)) {
        return false;
    }

    (*m)[message.key] = str;

    return true;
}

bool msg_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    Response *rsp = (Response *)field->message;

    if (field->tag == Response_types_tag) {
        MsgTypes *msg           = (MsgTypes *)field->pData;
        msg->types.funcs.decode = decode_types;
        msg->types.arg          = *arg;
    }

    return true;
}

bool encode_req(std::vector<uint8_t> *buffer, Request req)
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

bool decode_req(uint8_t *buffer, size_t len, Request *req)
{
    pb_istream_t stream = pb_istream_from_buffer(buffer, len);

    if (!pb_decode(&stream, Request_fields, req)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    return true;
}

void test_is_login()
{
    std::vector<uint8_t> buffer;
    Request enc   = Request_init_default;
    enc.func      = Functions_FUNC_IS_LOGIN;
    enc.which_msg = Request_empty_tag;

    encode_req(&buffer, enc);
    print_buffer(&buffer.front(), buffer.size());

    Request dec = Request_init_default;
    decode_req(&buffer.front(), buffer.size(), &dec);
    printf("Decoded message:\nFunc: %02x, ", dec.func);
    pb_release(Request_fields, &dec);
}

void test_get_self_wxid()
{
    std::vector<uint8_t> buffer;
    Request enc   = Request_init_default;
    enc.func      = Functions_FUNC_GET_SELF_WXID;
    enc.which_msg = Request_empty_tag;

    encode_req(&buffer, enc);
    print_buffer(&buffer.front(), buffer.size());

    Request dec = Request_init_default;
    decode_req(&buffer.front(), buffer.size(), &dec);
    printf("Decoded message:\nFunc: %02x, ", dec.func);
    pb_release(Request_fields, &dec);
}

void test_send_txt(char *msg, char *receiver, char *aters)
{
    std::vector<uint8_t> buffer;

    // TextMsg txt   = { msg, receiver, aters };
    Request enc   = Request_init_default;
    enc.func      = Functions_FUNC_SEND_TXT;
    enc.which_msg = Request_txt_tag;
    enc.msg.txt   = { msg, receiver, aters };

    encode_req(&buffer, enc);
    print_buffer(&buffer.front(), buffer.size());

    Request dec = Request_init_default;
    decode_req(&buffer.front(), buffer.size(), &dec);
    printf("Decoded message:\nFunc: %02x, ", dec.func);
    printf("\nmsg: %s\nreceiver: %s\naters: %s\n\n", dec.msg.txt.msg, dec.msg.txt.receiver, dec.msg.txt.aters);
    pb_release(Request_fields, &dec);
}

void test_get_msg_types()
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
    print_buffer(buffer, len);

    MsgTypes_t out;
    Response dec            = Response_init_default;
    pb_istream_t ostream    = pb_istream_from_buffer(buffer, len);
    dec.cb_msg.funcs.decode = msg_callback;
    dec.cb_msg.arg          = &out;
    if (!pb_decode(&ostream, Response_fields, &dec)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return;
    }
    printf("Decoded message:\nFunc: %02x, %02X\n", dec.func, dec.which_msg);

    for (auto it = out.begin(); it != out.end(); it++) {
        printf("key: %d, value: %s\r\n", it->first, it->second.c_str());
    }
    printf("Done.\n");
    pb_release(Response_fields, &dec);
}

int main()
{
    test_is_login();
    test_get_self_wxid();
    test_send_txt((char *)"This is message", (char *)"TO CHUCK", (char *)"@all");
    test_get_msg_types();

    return _getch();
}
