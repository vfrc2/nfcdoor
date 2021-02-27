#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

#define MAX_DEVICE_COUNT 16

static nfc_device *pnd = NULL;
static nfc_context *context;

static void stop_polling(int sig);

#define EXIT_FAILURE  1
#define EXIT_SUCCESS  0


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
    const size_t szModulations = 6;

    nfc_target nt;
    int res = 0;

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
        char * buff[10][10000];
        str_nfc_target(&buff, &nt, true);
        printf(buff[0]);
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

static void stop_polling(int sig)
{
  (void) sig;
  if (pnd != NULL)
    nfc_abort_command(pnd);
  else {
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
}