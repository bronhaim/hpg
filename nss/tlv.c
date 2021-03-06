#include <sys/types.h>
#include <arpa/inet.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "tlv.h"

#define TLV_TYPE_LENGTH		2
#define TLV_LENGTH_LENGTH	2

#define TLV_STATIC_SUPPORTED_OPTIONS_SIZE      13

enum tlv_opt_type tlv_static_supported_options[TLV_STATIC_SUPPORTED_OPTIONS_SIZE] = {
    TLV_OPT_MSG_SEQ_NUMBER,
    TLV_OPT_CLUSTER_NAME,
    TLV_OPT_TLS_SUPPORTED,
    TLV_OPT_TLS_CLIENT_CERT_REQUIRED,
    TLV_OPT_SUPPORTED_MESSAGES,
    TLV_OPT_SUPPORTED_OPTIONS,
    TLV_OPT_REPLY_ERROR_CODE,
    TLV_OPT_SERVER_MAXIMUM_REQUEST_SIZE,
    TLV_OPT_SERVER_MAXIMUM_REPLY_SIZE,
    TLV_OPT_NODE_ID,
    TLV_OPT_SUPPORTED_DECISION_ALGORITHMS,
    TLV_OPT_DECISION_ALGORITHM,
    TLV_OPT_HEARTBEAT_INTERVAL,
};

int
tlv_add(struct dynar *msg, enum tlv_opt_type opt_type, uint16_t opt_len, const void *value)
{
	uint16_t nlen;
	uint16_t nopt_type;

	if (dynar_size(msg) + sizeof(nopt_type) + sizeof(nlen) + opt_len > dynar_max_size(msg)) {
		return (-1);
	}

	nopt_type = htons((uint16_t)opt_type);
	nlen = htons(opt_len);

	dynar_cat(msg, &nopt_type, sizeof(nopt_type));
	dynar_cat(msg, &nlen, sizeof(nlen));
	dynar_cat(msg, value, opt_len);

	return (0);
}

int
tlv_add_u32(struct dynar *msg, enum tlv_opt_type opt_type, uint32_t u32)
{
	uint32_t nu32;

	nu32 = htonl(u32);

	return (tlv_add(msg, opt_type, sizeof(nu32), &nu32));
}

int
tlv_add_u8(struct dynar *msg, enum tlv_opt_type opt_type, uint8_t u8)
{

	return (tlv_add(msg, opt_type, sizeof(u8), &u8));
}

int
tlv_add_u16(struct dynar *msg, enum tlv_opt_type opt_type, uint16_t u16)
{
	uint16_t nu16;

	nu16 = htons(u16);

	return (tlv_add(msg, opt_type, sizeof(nu16), &nu16));
}

int
tlv_add_string(struct dynar *msg, enum tlv_opt_type opt_type, const char *str)
{

	return (tlv_add(msg, opt_type, strlen(str), str));
}

int
tlv_add_msg_seq_number(struct dynar *msg, uint32_t msg_seq_number)
{

	return (tlv_add_u32(msg, TLV_OPT_MSG_SEQ_NUMBER, msg_seq_number));
}

int
tlv_add_cluster_name(struct dynar *msg, const char *cluster_name)
{

	return (tlv_add_string(msg, TLV_OPT_CLUSTER_NAME, cluster_name));
}

int
tlv_add_tls_supported(struct dynar *msg, enum tlv_tls_supported tls_supported)
{

	return (tlv_add_u8(msg, TLV_OPT_TLS_SUPPORTED, tls_supported));
}

int
tlv_add_tls_client_cert_required(struct dynar *msg, int tls_client_cert_required)
{

	return (tlv_add_u8(msg, TLV_OPT_TLS_CLIENT_CERT_REQUIRED, tls_client_cert_required));
}

int
tlv_add_u16_array(struct dynar *msg, enum tlv_opt_type opt_type, const uint16_t *array, size_t array_size)
{
	size_t i;
	uint16_t *nu16a;
	uint16_t opt_len;
	int res;

	nu16a = malloc(sizeof(uint16_t) * array_size);
	if (nu16a == NULL) {
		return (-1);
	}

	for (i = 0; i < array_size; i++) {
		nu16a[i] = htons(array[i]);
	}

	opt_len = sizeof(uint16_t) * array_size;

	res = tlv_add(msg, opt_type, opt_len, nu16a);

	free(nu16a);

	return (res);
}

int
tlv_add_supported_options(struct dynar *msg, const enum tlv_opt_type *supported_options,
    size_t no_supported_options)
{
	uint16_t *u16a;
	size_t i;
	int res;

	u16a = malloc(sizeof(*u16a) * no_supported_options);
	if (u16a == NULL) {
		return (-1);
	}

	for (i = 0; i < no_supported_options; i++) {
		u16a[i] = (uint16_t)supported_options[i];
	}

	res = (tlv_add_u16_array(msg, TLV_OPT_SUPPORTED_OPTIONS, u16a, no_supported_options));

	free(u16a);

	return (res);
}

int
tlv_add_supported_decision_algorithms(struct dynar *msg, const enum tlv_decision_algorithm_type *supported_algorithms,
    size_t no_supported_algorithms)
{
	uint16_t *u16a;
	size_t i;
	int res;

	u16a = malloc(sizeof(*u16a) * no_supported_algorithms);
	if (u16a == NULL) {
		return (-1);
	}

	for (i = 0; i < no_supported_algorithms; i++) {
		u16a[i] = (uint16_t)supported_algorithms[i];
	}

	res = (tlv_add_u16_array(msg, TLV_OPT_SUPPORTED_DECISION_ALGORITHMS, u16a, no_supported_algorithms));

	free(u16a);

	return (res);
}

int
tlv_add_reply_error_code(struct dynar *msg, enum tlv_reply_error_code error_code)
{

	return (tlv_add_u16(msg, TLV_OPT_REPLY_ERROR_CODE, (uint16_t)error_code));
}

int
tlv_add_server_maximum_request_size(struct dynar *msg, size_t server_maximum_request_size)
{

	return (tlv_add_u32(msg, TLV_OPT_SERVER_MAXIMUM_REQUEST_SIZE, server_maximum_request_size));
}

int
tlv_add_server_maximum_reply_size(struct dynar *msg, size_t server_maximum_reply_size)
{

	return (tlv_add_u32(msg, TLV_OPT_SERVER_MAXIMUM_REPLY_SIZE, server_maximum_reply_size));
}

int
tlv_add_node_id(struct dynar *msg, uint32_t node_id)
{

	return (tlv_add_u32(msg, TLV_OPT_NODE_ID, node_id));
}

int
tlv_add_decision_algorithm(struct dynar *msg, enum tlv_decision_algorithm_type decision_algorithm)
{

	return (tlv_add_u16(msg, TLV_OPT_DECISION_ALGORITHM, (uint16_t)decision_algorithm));
}

int
tlv_add_heartbeat_interval(struct dynar *msg, uint32_t heartbeat_interval)
{

	return (tlv_add_u32(msg, TLV_OPT_HEARTBEAT_INTERVAL, heartbeat_interval));
}

void
tlv_iter_init(const struct dynar *msg, size_t msg_header_len, struct tlv_iterator *tlv_iter)
{

	tlv_iter->msg = msg;
	tlv_iter->current_pos = 0;
	tlv_iter->msg_header_len = msg_header_len;
}

enum tlv_opt_type
tlv_iter_get_type(const struct tlv_iterator *tlv_iter)
{
	uint16_t ntype;
	uint16_t type;

	memcpy(&ntype, dynar_data(tlv_iter->msg) + tlv_iter->current_pos, sizeof(ntype));
	type = ntohs(ntype);

	return (type);
}

uint16_t
tlv_iter_get_len(const struct tlv_iterator *tlv_iter)
{
	uint16_t nlen;
	uint16_t len;

	memcpy(&nlen, dynar_data(tlv_iter->msg) + tlv_iter->current_pos + TLV_TYPE_LENGTH, sizeof(nlen));
	len = ntohs(nlen);

	return (len);
}

const char *
tlv_iter_get_data(const struct tlv_iterator *tlv_iter)
{

	return (dynar_data(tlv_iter->msg) + tlv_iter->current_pos + TLV_TYPE_LENGTH + TLV_LENGTH_LENGTH);
}

int
tlv_iter_next(struct tlv_iterator *tlv_iter)
{
	uint16_t len;

	if (tlv_iter->current_pos == 0) {
		tlv_iter->current_pos = tlv_iter->msg_header_len;

		goto check_tlv_validity;
	}

	len = tlv_iter_get_len(tlv_iter);

	if (tlv_iter->current_pos + TLV_TYPE_LENGTH + TLV_LENGTH_LENGTH + len >= dynar_size(tlv_iter->msg)) {
		return (0);
	}

	tlv_iter->current_pos += TLV_TYPE_LENGTH + TLV_LENGTH_LENGTH + len;

check_tlv_validity:
	/*
	 * Check if tlv is valid = is not larger than whole message
	 */
	len = tlv_iter_get_len(tlv_iter);

	if (tlv_iter->current_pos + TLV_TYPE_LENGTH + TLV_LENGTH_LENGTH + len > dynar_size(tlv_iter->msg)) {
		return (-1);
	}

	return (1);
}

int
tlv_iter_decode_u32(struct tlv_iterator *tlv_iter, uint32_t *res)
{
	const char *opt_data;
	uint16_t opt_len;
	uint32_t nu32;

	opt_len = tlv_iter_get_len(tlv_iter);
	opt_data = tlv_iter_get_data(tlv_iter);

	if (opt_len != sizeof(nu32)) {
		return (-1);
	}

	memcpy(&nu32, opt_data, sizeof(nu32));
	*res = ntohl(nu32);

	return (0);
}

int
tlv_iter_decode_u8(struct tlv_iterator *tlv_iter, uint8_t *res)
{
	const char *opt_data;
	uint16_t opt_len;

	opt_len = tlv_iter_get_len(tlv_iter);
	opt_data = tlv_iter_get_data(tlv_iter);

	if (opt_len != sizeof(*res)) {
		return (-1);
	}

	memcpy(res, opt_data, sizeof(*res));

	return (0);
}

int
tlv_iter_decode_client_cert_required(struct tlv_iterator *tlv_iter, uint8_t *client_cert_required)
{

	return (tlv_iter_decode_u8(tlv_iter, client_cert_required));
}

int
tlv_iter_decode_str(struct tlv_iterator *tlv_iter, char **str, size_t *str_len)
{
	const char *opt_data;
	uint16_t opt_len;
	char *tmp_str;

	opt_len = tlv_iter_get_len(tlv_iter);
	opt_data = tlv_iter_get_data(tlv_iter);

	tmp_str = malloc(opt_len + 1);
	if (tmp_str == NULL) {
		return (-1);
	}

	memcpy(tmp_str, opt_data, opt_len);
	tmp_str[opt_len] = '\0';

	*str = tmp_str;
	*str_len = opt_len;

	return (0);
}

int
tlv_iter_decode_u16_array(struct tlv_iterator *tlv_iter, uint16_t **u16a, size_t *no_items)
{
	uint16_t opt_len;
	uint16_t *u16a_res;
	size_t i;

	opt_len = tlv_iter_get_len(tlv_iter);

	if (opt_len % sizeof(uint16_t) != 0) {
		return (-1);
	}

	*no_items = opt_len / sizeof(uint16_t);

	u16a_res = malloc(sizeof(uint16_t) * *no_items);
	if (u16a_res == NULL) {
		return (-2);
	}

	memcpy(u16a_res, tlv_iter_get_data(tlv_iter), opt_len);

	for (i = 0; i < *no_items; i++) {
		u16a_res[i] = ntohs(u16a_res[i]);
	}

	*u16a = u16a_res;

	return (0);
}

int
tlv_iter_decode_supported_options(struct tlv_iterator *tlv_iter, enum tlv_opt_type **supported_options,
    size_t *no_supported_options)
{
	uint16_t *u16a;
	enum tlv_opt_type *tlv_opt_array;
	size_t i;
	int res;

	res = tlv_iter_decode_u16_array(tlv_iter, &u16a, no_supported_options);
	if (res != 0) {
		return (res);
	}

	tlv_opt_array = malloc(sizeof(enum tlv_opt_type) * *no_supported_options);
	if (tlv_opt_array == NULL) {
		free(u16a);
		return (-2);
	}

	for (i = 0; i < *no_supported_options; i++) {
		tlv_opt_array[i] = (enum tlv_opt_type)u16a[i];
	}

	free(u16a);

	*supported_options = tlv_opt_array;

	return (0);
}

int
tlv_iter_decode_supported_decision_algorithms(struct tlv_iterator *tlv_iter,
    enum tlv_decision_algorithm_type **supported_decision_algorithms, size_t *no_supported_decision_algorithms)
{
	uint16_t *u16a;
	enum tlv_decision_algorithm_type *tlv_decision_algorithm_type_array;
	size_t i;
	int res;

	res = tlv_iter_decode_u16_array(tlv_iter, &u16a, no_supported_decision_algorithms);
	if (res != 0) {
		return (res);
	}

	tlv_decision_algorithm_type_array = malloc(
	    sizeof(enum tlv_decision_algorithm_type) * *no_supported_decision_algorithms);

	if (tlv_decision_algorithm_type_array == NULL) {
		free(u16a);
		return (-2);
	}

	for (i = 0; i < *no_supported_decision_algorithms; i++) {
		tlv_decision_algorithm_type_array[i] = (enum tlv_decision_algorithm_type)u16a[i];
	}

	free(u16a);

	*supported_decision_algorithms = tlv_decision_algorithm_type_array;

	return (0);
}

int
tlv_iter_decode_u16(struct tlv_iterator *tlv_iter, uint16_t *u16)
{
	const char *opt_data;
	uint16_t opt_len;
	uint16_t nu16;

	opt_len = tlv_iter_get_len(tlv_iter);
	opt_data = tlv_iter_get_data(tlv_iter);

	if (opt_len != sizeof(nu16)) {
		return (-1);
	}

	memcpy(&nu16, opt_data, sizeof(nu16));
	*u16 = ntohs(nu16);

	return (0);
}

int
tlv_iter_decode_reply_error_code(struct tlv_iterator *tlv_iter, enum tlv_reply_error_code *reply_error_code)
{

	return (tlv_iter_decode_u16(tlv_iter, (uint16_t *)reply_error_code));
}

int
tlv_iter_decode_tls_supported(struct tlv_iterator *tlv_iter, enum tlv_tls_supported *tls_supported)
{
	uint8_t u8;

	if (tlv_iter_decode_u8(tlv_iter, &u8) != 0) {
		return (-1);
	}

	*tls_supported = u8;

	if (*tls_supported != TLV_TLS_UNSUPPORTED &&
	    *tls_supported != TLV_TLS_SUPPORTED &&
	    *tls_supported != TLV_TLS_REQUIRED) {
		return (-4);
	}

	return (0);
}

int
tlv_iter_decode_decision_algorithm(struct tlv_iterator *tlv_iter, enum tlv_decision_algorithm_type *decision_algorithm)
{
	uint16_t u16;

	if (tlv_iter_decode_u16(tlv_iter, &u16) != 0) {
		return (-1);
	}

	*decision_algorithm = (enum tlv_decision_algorithm_type)u16;

	return (0);
}

void
tlv_get_supported_options(enum tlv_opt_type **supported_options, size_t *no_supported_options)
{

	*supported_options = tlv_static_supported_options;
	*no_supported_options = TLV_STATIC_SUPPORTED_OPTIONS_SIZE;
}
