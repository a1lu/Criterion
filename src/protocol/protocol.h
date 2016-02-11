/*
 * The MIT License (MIT)
 *
 * Copyright © 2015-2016 Franklin "Snaipe" Mathieu <http://snai.pe/>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*!
 * @file
 * @brief brief
 *****************************************************************************/
#ifndef PROTOCOL_H_
# define PROTOCOL_H_

# include <pb.h>
# include <pb_encode.h>
# include <pb_decode.h>
# include "criterion.pb.h"
# include "criterion/internal/preprocess.h"

enum protocol_version {
    PROTOCOL_V1 = 1,
};

extern volatile bool is_extern_worker;

# define criterion_message_set_id(Msg)                                      \
    do {                                                                    \
        if (is_extern_worker) {                                             \
            (Msg).id.uid = (char *) criterion_current_test->name;           \
        } else {                                                            \
            (Msg).id.pid = get_process_id();                                \
        }                                                                   \
    } while (0)

# define criterion_message(Kind, ...)                                       \
    (criterion_protocol_msg) {                                              \
        .version = PROTOCOL_V1,                                             \
        .which_id = is_extern_worker                                        \
                ? criterion_protocol_msg_uid_tag                            \
                : criterion_protocol_msg_pid_tag,                           \
        .data = {                                                           \
            .which_value = criterion_protocol_submessage_ ## Kind ## _tag,  \
            .value = {                                                      \
                .Kind = { __VA_ARGS__ },                                    \
            }                                                               \
        }                                                                   \
    }

#endif /* !PROTOCOL_H_ */
