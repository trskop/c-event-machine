/* Copyright (c) 2014, Peter Trško <peter.trsko@gmail.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Peter Trško nor the names of other
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EVENT_MACHINE_RESULT_INTERNAL_H_276692761618985232313111534537946615305
#define EVENT_MACHINE_RESULT_INTERNAL_H_276692761618985232313111534537946615305

#define not(x)                      (!(x))

#define null(x)                     ((x) == NULL)
#define if_null(x)                  if (null(x))
#define not_null(x)                 (not(null(x)))
#define if_not_null(x)              if (not_null(x))

#define is_negative(x)              ((x) < 0)
#define if_negative(x)              if (is_negative(x))
#define is_not_negative(x)          (not(is_negative(x)))
#define if_not_negative(x)          if (is_not_negative(x))

#define if_zero(x)                  if ((x) == 0)
#define if_not_zero(x)              if ((x) != 0)

#define valid_fd(fd)                (is_not_negative(fd))
#define if_valid_fd(fd)             if (valid_fd(fd))

#define invalid_fd(fd)              (is_negative(fd))
#define if_invalid_fd(fd)           if (invalid_fd(fd))

#define if_invalid_max_events(me)   if ((me) <= 0)

#endif
/* EVENT_MACHINE_RESULT_INTERNAL_H_276692761618985232313111534537946615305 */
