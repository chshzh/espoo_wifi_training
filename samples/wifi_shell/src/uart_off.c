/*
 * Not part of the upstream sample.
 *
 * "uart_off <seconds>": stop the shell and release the shell UART so it is suspended
 * by runtime PM for the requested time, then resume it. Use it to measure the
 * current of a connected, idle Wi-Fi link (for example DTIM wake-ups) without the
 * UART receiver adding to it.
 *
 * While the UART is off nothing can be typed or printed. Log output is dropped.
 */

#include <errno.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#define UART_OFF_MAX_SECONDS 3600
/* Time for the last shell output to leave the UART before it is suspended. */
#define UART_OFF_FLUSH_MS 200
#define UART_OFF_SETTLE_MS 100

static const struct device *const shell_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));

static struct k_work_delayable off_work;
static struct k_work_delayable on_work;
static uint32_t off_seconds;
static bool uart_is_off;

static void uart_on_handler(struct k_work *work)
{
	const struct shell *sh = shell_backend_uart_get_ptr();

	ARG_UNUSED(work);

	pm_device_runtime_get(shell_uart);
	/* The shell backend's RX interrupt callback is still registered. */
	uart_irq_rx_enable(shell_uart);
	shell_start(sh);
	uart_is_off = false;
	shell_print(sh, "UART is back on");
}

static void uart_off_handler(struct k_work *work)
{
	const struct shell *sh = shell_backend_uart_get_ptr();
	enum pm_device_state state;

	ARG_UNUSED(work);

	shell_stop(sh);
	/* shell_stop() leaves the backend's interrupts on. In interrupt-driven mode the
	 * UARTE driver holds a runtime PM reference while RX (or TX) interrupts are
	 * enabled, so disable them first.
	 */
	uart_irq_tx_disable(shell_uart);
	uart_irq_rx_disable(shell_uart);
	/* Drop the reference the shell UART backend took at init. If nobody else holds the
	 * device, runtime PM suspends it.
	 */
	pm_device_runtime_put(shell_uart);
	k_msleep(UART_OFF_SETTLE_MS);

	(void)pm_device_state_get(shell_uart, &state);
	if (state != PM_DEVICE_STATE_SUSPENDED) {
		/* Another user keeps the UART active: undo and report. */
		pm_device_runtime_get(shell_uart);
		uart_irq_rx_enable(shell_uart);
		shell_start(sh);
		shell_error(sh, "UART did not suspend (state %d), another user holds it", state);
		return;
	}

	uart_is_off = true;
	k_work_schedule(&on_work, K_SECONDS(off_seconds));
}

static int cmd_uart_off(const struct shell *sh, size_t argc, char *argv[])
{
	char *end;
	long seconds;

	ARG_UNUSED(argc);

	if (uart_is_off || k_work_delayable_is_pending(&off_work)) {
		shell_error(sh, "uart_off already in progress");
		return -EBUSY;
	}

	seconds = strtol(argv[1], &end, 10);
	if (*end != '\0' || seconds < 1 || seconds > UART_OFF_MAX_SECONDS) {
		shell_error(sh, "seconds must be 1-%d", UART_OFF_MAX_SECONDS);
		return -EINVAL;
	}

	off_seconds = (uint32_t)seconds;
	shell_print(sh, "UART off for %ld s. Output stops now and resumes automatically.",
		    seconds);
	k_work_schedule(&off_work, K_MSEC(UART_OFF_FLUSH_MS));

	return 0;
}

static int uart_off_init(void)
{
	k_work_init_delayable(&off_work, uart_off_handler);
	k_work_init_delayable(&on_work, uart_on_handler);

	return 0;
}

SYS_INIT(uart_off_init, APPLICATION, 90);

SHELL_CMD_ARG_REGISTER(uart_off, NULL,
		       "Turn the shell UART off for <seconds> (1-3600), then back on",
		       cmd_uart_off, 2, 0);
