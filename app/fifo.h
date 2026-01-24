#pragma once

#include <stdbool.h>
#include <stdint.h>

struct fifo8 {
	volatile uint8_t *data;
	volatile uint32_t capacity;
	volatile uint32_t head;
	volatile uint32_t num;
};

#define FIFO8_INIT(CAPACITY)           \
	{                              \
		.data = (uint8_t[]){}, \
		.capacity = CAPACITY,  \
		.head = 0,             \
		.num = 0,              \
	}

/**
 * fifo8_push:
 * @fifo: FIFO to push to
 * @data: data byte to push
 *
 * Push a data byte to the FIFO. Behaviour is undefined if the FIFO is full.
 * Clients are responsible for checking for fullness using fifo8_is_full().
 */
void fifo8_push(struct fifo8 *fifo, uint8_t data);

/**
 * fifo8_push_all:
 * @fifo: FIFO to push to
 * @data: data to push
 * @num: number of bytes to push
 *
 * Push a byte array to the FIFO. Behaviour is undefined if the FIFO is full.
 * Clients are responsible for checking the space left in the FIFO using
 * fifo8_num_free().
 */
void fifo8_push_all(struct fifo8 *fifo, const uint8_t *data, uint32_t num);

/**
 * fifo8_pop:
 * @fifo: fifo to pop from
 *
 * Pop a data byte from the FIFO. Behaviour is undefined if the FIFO is empty.
 * Clients are responsible for checking for emptyness using fifo8_is_empty().
 *
 * Returns: The popped data byte.
 */
uint8_t fifo8_pop(struct fifo8 *fifo);

/**
 * fifo8_peek:
 * @fifo: fifo to peek from
 *
 * Peek the data byte at the current head of the FIFO. Clients are responsible
 * for checking for emptyness using fifo8_is_empty().
 *
 * Returns: The peeked data byte.
 */
uint8_t fifo8_peek(struct fifo8 *fifo);

/**
 * fifo8_pop_buf:
 * @fifo: FIFO to pop from
 * @dest: the buffer to write the data into (can be NULL)
 * @destlen: size of @dest and maximum number of bytes to pop
 *
 * Pop a number of elements from the FIFO up to a maximum of @destlen.
 * The popped data is copied into the @dest buffer.
 * Care is taken when the data wraps around in the ring buffer.
 *
 * Returns: number of bytes popped.
 */
uint32_t fifo8_pop_buf(struct fifo8 *fifo, uint8_t *dest, uint32_t destlen);

/**
 * fifo8_peek_buf:
 * @fifo: FIFO to read from
 * @dest: the buffer to write the data into (can be NULL)
 * @destlen: size of @dest and maximum number of bytes to peek
 *
 * Peek a number of elements from the FIFO up to a maximum of @destlen.
 * The peeked data is copied into the @dest buffer.
 * Care is taken when the data wraps around in the ring buffer.
 *
 * Returns: number of bytes peeked.
 */
uint32_t fifo8_peek_buf(struct fifo8 *fifo, uint8_t *dest, uint32_t destlen);

/**
 * fifo8_pop_bufptr:
 * @fifo: FIFO to pop from
 * @max: maximum number of bytes to pop
 * @numptr: pointer filled with number of bytes returned (can be NULL)
 *
 * New code should prefer to use fifo8_pop_buf() instead of fifo8_pop_bufptr().
 *
 * Pop a number of elements from the FIFO up to a maximum of @max. The buffer
 * containing the popped data is returned. This buffer points directly into
 * the internal FIFO backing store and data (without checking for overflow!)
 * and is invalidated once any of the fifo8_* APIs are called on the FIFO.
 *
 * The function may return fewer bytes than requested when the data wraps
 * around in the ring buffer; in this case only a contiguous part of the data
 * is returned.
 *
 * The number of valid bytes returned is populated in *@numptr; will always
 * return at least 1 byte. max must not be 0 or greater than the number of
 * bytes in the FIFO.
 *
 * Clients are responsible for checking the availability of requested data
 * using fifo8_num_used().
 *
 * Returns: A pointer to popped data.
 */
const uint8_t *fifo8_pop_bufptr(struct fifo8 *fifo, uint32_t max,
				uint32_t *numptr);

/**
 * fifo8_peek_bufptr: read upto max bytes from the fifo
 * @fifo: FIFO to read from
 * @max: maximum number of bytes to peek
 * @numptr: pointer filled with number of bytes returned (can be NULL)
 *
 * Peek into a number of elements from the FIFO up to a maximum of @max.
 * The buffer containing the data peeked into is returned. This buffer points
 * directly into the FIFO backing store. Since data is invalidated once any
 * of the fifo8_* APIs are called on the FIFO, it is the caller responsibility
 * to access it before doing further API calls.
 *
 * The function may return fewer bytes than requested when the data wraps
 * around in the ring buffer; in this case only a contiguous part of the data
 * is returned.
 *
 * The number of valid bytes returned is populated in *@numptr; will always
 * return at least 1 byte. max must not be 0 or greater than the number of
 * bytes in the FIFO.
 *
 * Clients are responsible for checking the availability of requested data
 * using fifo8_num_used().
 *
 * Returns: A pointer to peekable data.
 */
const uint8_t *fifo8_peek_bufptr(struct fifo8 *fifo, uint32_t max,
				 uint32_t *numptr);

/**
 * fifo8_drop:
 * @fifo: FIFO to drop bytes
 * @len: number of bytes to drop
 *
 * Drop (consume) bytes from a FIFO.
 */
void fifo8_drop(struct fifo8 *fifo, uint32_t len);

/**
 * fifo8_reset:
 * @fifo: FIFO to reset
 *
 * Reset a FIFO. All data is discarded and the FIFO is emptied.
 */
void fifo8_reset(struct fifo8 *fifo);

/**
 * fifo8_is_empty:
 * @fifo: FIFO to check
 *
 * Check if a FIFO is empty.
 *
 * Returns: True if the fifo is empty, false otherwise.
 */
bool fifo8_is_empty(struct fifo8 *fifo);

/**
 * fifo8_is_full:
 * @fifo: FIFO to check
 *
 * Check if a FIFO is full.
 *
 * Returns: True if the fifo is full, false otherwise.
 */
bool fifo8_is_full(struct fifo8 *fifo);

/**
 * fifo8_num_free:
 * @fifo: FIFO to check
 *
 * Return the number of free bytes in the FIFO.
 *
 * Returns: Number of free bytes.
 */
uint32_t fifo8_num_free(struct fifo8 *fifo);

/**
 * fifo8_num_used:
 * @fifo: FIFO to check
 *
 * Return the number of used bytes in the FIFO.
 *
 * Returns: Number of used bytes.
 */
uint32_t fifo8_num_used(struct fifo8 *fifo);

extern void fifo_selftest(void);
