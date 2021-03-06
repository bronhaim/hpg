#include <stdio.h>
#include <nss.h>
#include <secerr.h>
#include <sslerr.h>
#include <pk11func.h>
#include <certt.h>
#include <ssl.h>
#include <prio.h>
#include <private/pprio.h>
#include <prnetdb.h>
#include <prerror.h>
#include <prinit.h>
#include <getopt.h>
#include <err.h>
#include <keyhi.h>
#include <unistd.h>

#include "nss-sock.h"

#define NSS_DB_DIR	"node/nssdb"

#define ENABLE_TLS	1

PRFileDesc *client_socket;

static void err_nss(void) {
	errx(1, "nss error %d: %s", PR_GetError(), PR_ErrorToString(PR_GetError(), PR_LANGUAGE_I_DEFAULT));
}

/*static void warn_nss(void) {
	warnx("nss error %d: %s", PR_GetError(), PR_ErrorToString(PR_GetError(), PR_LANGUAGE_I_DEFAULT));
}*/

static SECStatus nss_bad_cert_hook(void *arg, PRFileDesc *fd) {
	if (PR_GetError() == SEC_ERROR_EXPIRED_CERTIFICATE || PR_GetError() == SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE ||
	    PR_GetError() == SEC_ERROR_CRL_EXPIRED || PR_GetError() == SEC_ERROR_KRL_EXPIRED ||
	    PR_GetError() == SSL_ERROR_EXPIRED_CERT_ALERT) {
		fprintf(stderr, "Expired certificate\n");
		return (SECSuccess);
	}

//	warn_nss();

	return (SECFailure);
}

static char *get_pwd(PK11SlotInfo *slot, PRBool retry, void *arg)
{
	FILE *f;
	char pwd[255];

	if (retry) {
		return (NULL);
	}

	f = fopen(NSS_DB_DIR"/pwdfile.txt", "rt");
	if (f == NULL) {
		err(1, "Can't open pwd file");
	}

	fgets(pwd, sizeof(pwd), f);
	fclose(f);

	if (pwd[strlen(pwd) - 1] == '\n')
		pwd[strlen(pwd) - 1] = '\0';

	fprintf(stderr, "Return %s password\n", pwd);

	return (PL_strdup(pwd));
}

int
recv_from_server(PRFileDesc *socket)
{
	char buf[255];
	PRInt32 readed;

	fprintf(stderr, "PR_READ\n");
	readed = PR_Recv(socket, buf, sizeof(buf), 0, 1000);
	fprintf(stderr, "-PR_READ %u\n", readed);
	if (readed > 0) {
		buf[readed] = '\0';
		printf("Client %p readed %u bytes: %s\n", socket, readed, buf);
	}

	if (readed == 0) {
		printf("Client %p EOF\n", socket);
	}

	if (readed < 0 && PR_GetError() == PR_WOULD_BLOCK_ERROR) {
		fprintf(stderr, "WOULD BLOCK\n");
	}

	if (readed < 0 && PR_GetError() != PR_IO_TIMEOUT_ERROR && PR_GetError() != PR_WOULD_BLOCK_ERROR) {
		err_nss();
	}

	return (readed);
}

SECStatus get_client_auth_data(void *arg,
    PRFileDesc *socket,
    struct CERTDistNamesStr *caNames,
    struct CERTCertificateStr **pRetCert,
    struct SECKEYPrivateKeyStr **pRetKey)
{
    fprintf(stderr, "Client cert requested\n");
//    sleep(4);
    return (NSS_GetClientAuthData(arg, socket, caNames, pRetCert, pRetKey));
}

void
handle_client(PRFileDesc *socket)
{
	PRPollDesc pfds[2];
	PRInt32 res;
	int exit_loop;
	char to_send[255];
	PRInt32 sent;
	PRFileDesc *prstdin;

	exit_loop = 0;

	if ((prstdin = PR_CreateSocketPollFd(STDIN_FILENO)) == NULL) {
		err_nss();
	}

	while (!exit_loop) {
		fprintf(stderr,"Handle client loop\n");
		pfds[0].fd = prstdin;
		pfds[0].in_flags = PR_POLL_READ | PR_POLL_EXCEPT;
		pfds[0].out_flags = 0;
		pfds[1].fd = socket;
		pfds[1].in_flags = PR_POLL_READ | PR_POLL_EXCEPT;
		pfds[1].out_flags = 0;

		if ((res = PR_Poll(pfds, 2, PR_INTERVAL_NO_TIMEOUT)) > 0) {
			if (pfds[0].out_flags & PR_POLL_READ) {
				if (fgets(to_send, sizeof(to_send), stdin) == NULL) {
					exit_loop = 1;
					break;
				}

				if ((sent = PR_Send(socket, to_send, strlen(to_send), 0, PR_INTERVAL_NO_TIMEOUT)) == -1) {
					err_nss();
				}
				fprintf(stderr,"sent = %u\n", sent);

#ifdef ENABLE_TLS
				if (strcmp(to_send, "starttls\n") == 0) {
					int reset_would_block;

					if ((client_socket = nss_sock_start_ssl_as_client(client_socket, "Qnetd Server",
					    nss_bad_cert_hook, /*NSS_GetClientAuthData*/ get_client_auth_data,
					    "Cluster Cert", 1, &reset_would_block)) == NULL) {
						fprintf(stderr, "AAAAA\n");
						err_nss();
					}
				}
#endif
			}

			if (pfds[1].out_flags & PR_POLL_READ) {
				if (recv_from_server(pfds[1].fd) == 0) {
					exit_loop = 1;
				}
			}

			if (pfds[1].out_flags & PR_POLL_ERR) {
				fprintf(stderr, "ERR\n");
			}

			if (pfds[1].out_flags & PR_POLL_NVAL) {
				fprintf(stderr, "NVAL\n");
			}

			if (pfds[1].out_flags & PR_POLL_HUP) {
				fprintf(stderr, "HUP\n");
			}

			if (pfds[1].out_flags & PR_POLL_EXCEPT) {
				fprintf(stderr, "EXCEPT\n");
			}
		}
	}

	if (PR_DestroySocketPollFd(prstdin) != PR_SUCCESS) {
		err_nss();
	}
}


int main(void)
{

	if (nss_sock_init_nss(NSS_DB_DIR) != 0) {
		err_nss();
	}

	PK11_SetPasswordFunc(get_pwd);

	client_socket = nss_sock_create_client_socket("localhost", 4433, PR_AF_UNSPEC, 100);
	if (client_socket == NULL) {
		err_nss();
	}

#ifndef ENABLE_TLS
	if ((client_socket = nss_sock_start_ssl_as_client(client_socket, "Qnetd Server", nss_bad_cert_hook,
	    get_client_auth_data, "Cluster Cert")) == NULL) {
		fprintf(stderr, "AAAAA\n");
		err_nss();
	}
#endif

	handle_client(client_socket);

	if (PR_Close(client_socket) != PR_SUCCESS) {
		err_nss();
	}

	SSL_ClearSessionCache();

	if (NSS_Shutdown() != SECSuccess) {
		err_nss();
	}

	PR_Cleanup();

	return (0);
}
