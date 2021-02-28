#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

#define MAX_DEVICE_COUNT 16

static nfc_device *pnd = NULL;
static nfc_context *context;

static void stop_polling(int sig);
int cardTransmit(nfc_device *pnd, uint8_t *capdu, size_t capdulen, uint8_t *rapdu, size_t *rapdulen);

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

int main()
{
    signal(SIGINT, stop_polling);

    const uint8_t uiPollNr = 20;
    const uint8_t uiPeriod = 2;
    const nfc_modulation nmModulations[6] = {
        {.nmt = NMT_ISO14443A, .nbr = NBR_106},
        {.nmt = NMT_ISO14443B, .nbr = NBR_106},
        {.nmt = NMT_FELICA, .nbr = NBR_212},
        {.nmt = NMT_FELICA, .nbr = NBR_424},
        {.nmt = NMT_JEWEL, .nbr = NBR_106},
        // {.nmt = NMT_ISO14443BICLASS, .nbr = NBR_106},
    };
    const size_t szModulations = 5;

    nfc_target nt;
    int res = 0;

    const char *acLibnfcVersion = nfc_version();

    printf("libnfc %s\n", acLibnfcVersion);

    nfc_init(&context);
    if (context == NULL)
    {
        printf("Unable to init libnfc (malloc)");
        exit(EXIT_FAILURE);
    }

    pnd = nfc_open(context, NULL);

    if (pnd == NULL)
    {
        printf("%s", "Unable to open NFC device.");
        nfc_exit(context);
        exit(EXIT_FAILURE);
    }

    if (nfc_initiator_init(pnd) < 0)
    {
        nfc_perror(pnd, "nfc_initiator_init");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
    }

    nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, true);

    printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));
    printf("NFC device will poll during %ld ms (%u pollings of %lu ms for %ld modulations)\n", (unsigned long)uiPollNr * szModulations * uiPeriod * 150, uiPollNr, (unsigned long)uiPeriod * 150, szModulations);
    if ((res = nfc_initiator_poll_target(pnd, nmModulations, szModulations, uiPollNr, uiPeriod, &nt)) < 0)
    {
        nfc_perror(pnd, "nfc_initiator_poll_target");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
    }

    if (res > 0)
    {
        printf("Target detected!\n");
        char *buff[10][10000];
        str_nfc_target(&buff, &nt, true);
        printf("%s\n", buff[0][0]);
        fflush(stdout);

        uint8_t capdu[264];
        size_t capdulen;
        uint8_t rapdu[264];
        size_t rapdulen;
        // Select application
        char *apdu_cmd = "\x00\xA4\x04\x00\x07";
        // char *apdu_data = "\xd2\x76\x00\x00\x85\x01\x00";
        char *apdu_data = "\xF0\x01\x02\x03\x04\x05\x06";
        memcpy(capdu, apdu_cmd, 5);
        memcpy(capdu + 5, apdu_data, 7);
        capdulen = 12;
        rapdulen = sizeof(rapdu);
        if (cardTransmit(pnd, capdu, capdulen, rapdu, &rapdulen) < 0)
            exit(EXIT_FAILURE);
        // if (rapdulen < 2 || rapdu[rapdulen - 2] != 0x90 || rapdu[rapdulen - 1] != 0x00)
        //     exit(EXIT_FAILURE);
        printf("Application selected!\n");

        memcpy(capdu, "\x00\x00\x00\x00\x00\x00\x00", 7);
        capdulen = 7;

        for (int i = 0; i < 10; i++) {
            printf("Read byte #%i\n", i);
            rapdulen = sizeof(rapdu);
            if (cardTransmit(pnd, capdu, capdulen, rapdu, &rapdulen) < 0) {
                printf("Read error");
                exit(EXIT_FAILURE);
            }
            rapdu[rapdulen] = 0;
            printf("%s\n", rapdu);
        }



        printf("Waiting for card removing...");
        fflush(stdout);
        while (0 == nfc_initiator_target_is_present(pnd, NULL))
        {
        }
        nfc_perror(pnd, "nfc_initiator_target_is_present");
        printf("done.\n");
    }
    else
    {
        printf("No target found.\n");
    }

    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_SUCCESS);
}

int cardTransmit(nfc_device *pnd, uint8_t *capdu, size_t capdulen, uint8_t *rapdu, size_t *rapdulen)
{
    int res;
    size_t szPos;
    printf("=> ");
    for (szPos = 0; szPos < capdulen; szPos++)
    {
        printf("%02x ", capdu[szPos]);
    }
    printf("\n");
    if ((res = nfc_initiator_transceive_bytes(pnd, capdu, capdulen, rapdu, *rapdulen, 500)) < 0)
    {
        return -1;
    }
    else
    {
        *rapdulen = (size_t)res;
        printf("<= ");
        for (szPos = 0; szPos < *rapdulen; szPos++)
        {
            printf("%02x ", rapdu[szPos]);
        }
        printf("\n");
        return 0;
    }
}

static void stop_polling(int sig)
{
    (void)sig;
    if (pnd != NULL)
        nfc_abort_command(pnd);
    else
    {
        nfc_exit(context);
        exit(EXIT_FAILURE);
    }
}