#pragma once

/**
 * @file glia.h
 * @brief Main header for GliaGL - Spiking Neural Network Library
 * 
 * Include this file to access the complete GliaGL API.
 * 
 * Example usage:
 * @code
 * #include <glia/glia.h>
 * 
 * int main() {
 *     glia::Network net;
 *     net.load("network.net");
 *     
 *     glia::Trainer trainer(std::make_shared<glia::Network>(net));
 *     // ... training code ...
 * }
 * @endcode
 */

// Public API headers
#include "types.h"
#include "network.h"
#include "trainer.h"
#include "evolution.h"

/**
 * @namespace glia
 * @brief GliaGL public API namespace
 * 
 * All public-facing classes and functions are in this namespace.
 * Internal implementation details are in the global namespace.
 */
namespace glia {

/**
 * @brief Get library version string
 * @return Version in format "MAJOR.MINOR.PATCH"
 */
const char* getVersion();

/**
 * @brief Initialize GliaGL library (optional)
 * 
 * Can be called to pre-initialize any library resources.
 * Not required for basic usage.
 */
void initialize();

} // namespace glia
