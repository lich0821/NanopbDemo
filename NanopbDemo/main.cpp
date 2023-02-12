#include <conio.h> // _getch()
#include <stdio.h>
#include <vector>

#include <pb_decode.h>
#include <pb_encode.h>

#include "demo.pb.h"

void print_buffer(uint8_t *buffer, size_t len)
{
    printf("Encoded message[%d]:\n", len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
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
    Request enc = Request_init_default;
    enc.func = Functions_FUNC_IS_LOGIN;
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
    Request enc = Request_init_default;
    enc.func = Functions_FUNC_GET_SELF_WXID;
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

int main()
{
    test_is_login();
    test_get_self_wxid();
    test_send_txt((char *)"This is message", (char *)"TO CHUCK", (char *)"@all");

    return _getch();
}
