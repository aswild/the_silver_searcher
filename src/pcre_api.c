/******************************************************************************
 * Copyright (c) 2016 Allen Wild <allenwild93@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#include "pcre_api.h"

const char *ag_pcre_version(void) {
#ifndef HAVE_PCRE2
    return pcre_version();
#else
    return AG_STRINGIFY(PCRE2_MAJOR) "." AG_STRINGIFY(PCRE2_MINOR) " " AG_STRINGIFY(PCRE2_DATE)
#endif
}
