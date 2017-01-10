/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Host command module for Chrome EC */

#ifndef __CROS_EC_HOST_COMMAND_H
#define __CROS_EC_HOST_COMMAND_H

#include "common.h"
#include "ec_commands.h"

/* Args for host command handler */
struct host_cmd_handler_args {
	/*
	 * The driver that receives the command sets up the send_response()
	 * handler. Once the command is processed this handler is called to
	 * send the response back to the host.
	 */
	void (*send_response)(struct host_cmd_handler_args *args);
	uint16_t command;      /* Command (e.g., EC_CMD_FLASH_GET_INFO) */
	uint8_t version;       /* Version of command (0-31) */

	const void *params; /* Input parameters */
	uint16_t params_size;  /* Size of input parameters in bytes */

	/*
	 * Pointer to output response data buffer.  On input to the handler,
	 * points to a buffer of size response_max.
	 */
	void *response;

	/* Maximum size of response buffer provided to command handler */
	uint16_t response_max;

	/*
	 * Size of data pointed to by response.  Defaults to 0, so commands
	 * which do not produce response data do not need to set this.
	 */
	uint16_t response_size;

	/*
	 * This is the result returned by command and therefore the status to
	 * be reported from the command execution to the host. The driver
	 * should set this to EC_RES_SUCCESS on receipt of a valid command.
	 * It is then passed back to the driver via send_response() when
	 * command execution is complete. The driver may still override this
	 * when sending the response back to the host if it detects an error
	 * in the response or in its own operation.
	 */
	enum ec_status result;
};

/* Args for host packet handler */
struct host_packet {
	/*
	 * The driver that receives the command sets up the send_response()
	 * handler. Once the command is processed this handler is called to
	 * send the response back to the host.
	 */
	void (*send_response)(struct host_packet *pkt);

	/* Input request data */
	const void *request;

	/*
	 * Input request temp buffer.  If this is non-null, the data has not
	 * been copied from here into the request buffer yet.  The host command
	 * handler should do so while verifying the command.  The interface
	 * can't do it, because it doesn't know how much to copy.
	 */
	void *request_temp;

	/*
	 * Maximum size of request the interface can handle, in bytes.  The
	 * buffers pointed to by *request and *request_temp must be at least
	 * this big.
	 */
	uint16_t request_max;

	/* Size of input request data, in bytes */
	uint16_t request_size;

	/* Pointer to output response data buffer */
	void *response;

	/* Maximum size of response buffer provided to command handler */
	uint16_t response_max;

	/* Size of output response data, in bytes */
	uint16_t response_size;

	/*
	 * Error from driver; if this is non-zero, host command handler will
	 * return a properly formatted error response packet rather than
	 * calling a command handler.
	 */
	enum ec_status driver_result;
};

/* Host command */
struct host_command {
	/*
	 * Handler for the command.  Args points to context for handler.
	 * Returns result status (EC_RES_*).
	 */
	int (*handler)(struct host_cmd_handler_args *args);
	/* Command code */
	int command;
	/* Mask of supported versions */
	int version_mask;
};

/**
 * Return a pointer to the memory-mapped buffer.
 *
 * This buffer is EC_MEMMAP_SIZE bytes long, is writable at any time, and the
 * host can read it at any time.
 *
 * @param offset        Offset within the range to return
 * @return pointer to the buffer at that offset
 */
uint8_t *host_get_memmap(int offset);

/**
 * Process a host command and return its response
 *
 * @param args	        Command handler args
 * @return resulting status
 */
enum ec_status host_command_process(struct host_cmd_handler_args *args);

#ifdef CONFIG_HOSTCMD_EVENTS
/**
 * Set one or more host event bits.
 *
 * @param mask          Event bits to set (use EC_HOST_EVENT_MASK()).
 */
void host_set_events(uint32_t mask);

/**
 * Set a single host event.
 *
 * @param event         Event to set (EC_HOST_EVENT_*).
 */
static inline void host_set_single_event(int event)
{
	host_set_events(EC_HOST_EVENT_MASK(event));
}

/**
 * Clear one or more host event bits.
 *
 * @param mask          Event bits to clear (use EC_HOST_EVENT_MASK()).
 *                      Write 1 to a bit to clear it.
 */
void host_clear_events(uint32_t mask);

/**
 * Return the raw event state.
 */
uint32_t host_get_events(void);
#endif

/**
 * Send a response to the relevant driver for transmission
 *
 * Once command processing is complete, this is used to send a response
 * back to the host.
 *
 * @param args	Contains response to send
 */
void host_send_response(struct host_cmd_handler_args *args);

/**
 * Called by host interface module when a command is received.
 */
void host_command_received(struct host_cmd_handler_args *args);

/**
 * Return the expected host packet size given its header.
 *
 * Also does some sanity checking on the host request.
 *
 * @param r		Host request header
 * @return The expected packet size, or 0 if error.
 */
int host_request_expected_size(const struct ec_host_request *r);

/**
 * Handle a received host packet.
 *
 * @param packet	Host packet args
 */
void host_packet_receive(struct host_packet *pkt);

#ifdef HAS_TASK_HOSTCMD
#define EXPAND(off, cmd) __host_cmd_(off, cmd)
#define __host_cmd_(off, cmd) __host_cmd_##off##cmd
#define EXPANDSTR(off, cmd) "__host_cmd_"#off#cmd

/*
 * Register a host command handler with
 * commands starting at offset 0x0000
 */
#define DECLARE_HOST_COMMAND(command, routine, version_mask)    \
	const struct host_command __keep EXPAND(0x0000, command)        \
	__attribute__((section(".rodata.hcmds."EXPANDSTR(0x0000, command)))) \
		= {routine, command, version_mask}

/*
 * Register a private host command handler with
 * commands starting at offset EC_CMD_BOARD_SPECIFIC_BASE,
 */
#define DECLARE_PRIVATE_HOST_COMMAND(command, routine, version_mask) \
	const struct host_command __keep \
	EXPAND(EC_CMD_BOARD_SPECIFIC_BASE, command) \
	__attribute__((section(".rodata.hcmds."\
	EXPANDSTR(EC_CMD_BOARD_SPECIFIC_BASE, command)))) \
		= {routine, EC_PRIVATE_HOST_COMMAND_VALUE(command), \
		   version_mask}
#else
#define DECLARE_HOST_COMMAND(command, routine, version_mask)    \
	int (routine)(struct host_cmd_handler_args *args)       \
		__attribute__((unused))

#define DECLARE_PRIVATE_HOST_COMMAND(command, routine, version_mask)	\
	DECLARE_HOST_COMMAND(command, routine, version_mask)
#endif

/**
 * Politely ask the CPU to enable/disable its own throttling.
 *
 * @param throttle	Enable (!=0) or disable(0) throttling
 */
void host_throttle_cpu(int throttle);


/**
 * Signal host command task to send status to PD MCU.
 *
 * @new_chg_state PD MCU charge state
 */
void host_command_pd_send_status(enum pd_charge_state new_chg_state);

/**
 * Signal host command task to inform PD MCU that the EC is going to hibernate,
 * which will normally cause the PD MCU to hibernate also.
 */
void host_command_pd_request_hibernate(void);

/**
 * Get the active charge port from the PD
 *
 * @return -1 == none/unknown, 0 == left, 1 == right.
 */
int pd_get_active_charge_port(void);

/**
 * Send host command to PD MCU.
 *
 * @param command Host command number
 * @param version Version of host command
 * @param outdata Pointer to buffer of out data
 * @param outsize Size of buffer to out data
 * @param indata Pointer to buffer to store response
 * @param insize Size of buffer to store response
 */
int pd_host_command(int command, int version,
		    const void *outdata, int outsize,
		    void *indata, int insize);


/**
 * EC: Get verify boot mode
 * @return vboot_mode as the following:
 *    VBOOT_MODE_NORMAL    - normal mode
 *    VBOOT_MODE_DEVELOPER - developer mode
 *    VBOOT_MODE_RECOVERY  - recovery mode
 */
int host_get_vboot_mode(void);

#endif  /* __CROS_EC_HOST_COMMAND_H */
