#pragma once

#define MESSAGES_LEN (16384)

#include "span.h"

usize read_messages(span_t*, char*);
