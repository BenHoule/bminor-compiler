#pragma once

extern bool resolver_trace;  // Toggles printing binding trace

unsigned int resolver_error_count();
void resolver_error(const char *fmt, ...);
