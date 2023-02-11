#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

#include <pb_decode.h>
#include <pb_encode.h>

#include "demo.pb.h"

int main()
{
    /* This is the buffer where we will store our message. */
    uint8_t *buffer;
    size_t message_length;
    bool status;

    /* Encode our message */
    {
        /* Allocate space on the stack to store the message data.
         *
         * Nanopb generates simple struct definitions for all the messages.
         * - check out the contents of simple.pb.h!
         * It is a good idea to always initialize your structures
         * so that you do not have garbage data from RAM in there.
         */
        Response message = Response_init_zero;

        /* Fill in the code */
        message.code = 13;
        if (!pb_get_encoded_size(&message_length, Response_fields, &message)) {
            return -1;
        }
        printf("Encoded size: %d\n", message_length);

        buffer = (uint8_t *)malloc(message_length);
        if (buffer == NULL) {
            return -2;
        }

        /* Create a stream that will write to our buffer. */
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        /* Now we are ready to encode the message! */
        status = pb_encode(&stream, Response_fields, &message);

        /* Then just check for any errors.. */
        if (!status) {
            printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }
    }

    printf("Encoded message: ");
    for (size_t i = 0; i < message_length; i++) {
        printf("%02X ", (uint8_t)buffer[i]);
    }
    printf("\n");

    /* Now we could transmit the message over network, store it in a file or
     * wrap it to a pigeon's leg.
     */

    /* But because we are lazy, we will just decode it immediately. */

    {
        /* Allocate space for the decoded message. */
        Response message = Response_init_zero;

        /* Create a stream that reads from the buffer. */
        pb_istream_t stream = pb_istream_from_buffer(buffer, message_length);

        /* Now we are ready to decode the message. */
        status = pb_decode(&stream, Response_fields, &message);

        /* Check for errors... */
        if (!status) {
            printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        /* Print the data contained in the message. */
        printf("The response code is: %d\n", (int)message.code);
        free(buffer);
    }

    return _getch();
}
