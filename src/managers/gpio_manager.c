#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "managers/gpio_manager.h"
#include "managers/ui_manager.h"

LOG_MODULE_REGISTER(GPIO, LOG_LEVEL_INF);


/**
 * @brief PG pin of the MCP73871
 *
 * @param gpio_pg Node for the gpio pin defined
 *        in the overlay file for the board
 * @param gpios Internal zephyr gpios label 
 *
 * @return 
 */
static const struct gpio_dt_spec pg_gpio = 
            GPIO_DT_SPEC_GET(DT_NODELABEL(gpio_pg),
                            gpios);

/**
 * @brief Wakeup button on the PCB
 *
 * @param gpio_wakeup Node for gpio pin defined in 
 *        overlay
 * @param gpios Internal zephyr gpio label
 *
 * @return 
 */
static const struct gpio_dt_spec wakeup_gpio = 
            GPIO_DT_SPEC_GET(DT_NODELABEL(gpio_wakeup),
                            gpios);
/**
 * @brief Call back struct required for internal 
 * zephyr linked list, cant be placed in stack so
 * define it in text section by static
 */
static struct gpio_callback pg_gpio_cb_data;

/**
 * @brief Call back struct required for internal 
 * zephyr linked list, cant be placed in stack so
 * define it in text section by static
 */
static struct gpio_callback wakeup_gpio_cb_data;


/**
 * @brief Call back function for GPIO state changes
 *
 * @param dev Pointer to gpio controller device structure
 * there might be multiple gpio devices in header file
 * this helps to identify which one called this
 * This function runs in IRQ context and hust to retun fast
 * Offload heavy tasks to threads or k_work
 * @param cb gpio_callback structure associated with 
 * this GPIO pin
 * @param pins Bitmask of the pins of the GPIO port 
 */
void power_state_changed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    /* Determine which pin triggered the callback */
    if (pins & BIT(pg_gpio.pin)) {
        LOG_INF("PG gpio changed");
    }
    if (pins & BIT(wakeup_gpio.pin)) {
        LOG_INF("Wakeup gpio changed");
        // If we are in deep sleep this should wakeup 
        // device and restart the device from new
        // If we are in background mode, this should 
        // switch to active mode and turn on display
        // If we are already in active mode this should 
        // ignore the callback 
        init_ui();
    }
    
}

/**
 * @brief Add interrupts for each gpio_pin
 *
 * @param gpio GPIO on which interrupt is created
 * @param cb_data gpio_callback struct for this pin
 */
void config_gpio_pin_as_interrupt(const struct gpio_dt_spec *gpio,
                       struct gpio_callback *cb_data,
                       gpio_flags_t activation) {
    gpio_pin_configure_dt(gpio, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(gpio, 
                                    activation);
    gpio_init_callback(cb_data, 
                        power_state_changed,
                        BIT(gpio->pin));
    gpio_add_callback(gpio->port,
                      cb_data);
}



/**
 * @brief First setup of GPIO pins
 */
void init_gpio_pins() {

    //config_gpio_pin_as_interrupt(&pg_gpio, 
    //                           &pg_gpio_cb_data,
    //                           GPIO_INT_EDGE_BOTH);
    //config_gpio_pin_as_interrupt(&wakeup_gpio,
    //                           &wakeup_gpio_cb_data,
    //                           GPIO_INT_EDGE_RISING);
}
