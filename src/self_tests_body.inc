/*
 * Regression suite coordinator for invariants across access control, validation,
 * generation, materialization, and formatting.
 *
 * The driver remains isolated so follow-up PRs can extract subsystem-specific
 * blocks from self_tests_driver.inc into narrower include files without
 * changing the self_tests.cpp translation-unit boundary.
 */
#include "self_tests_driver.inc"
