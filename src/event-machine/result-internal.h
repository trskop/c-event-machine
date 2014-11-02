/* Copyright (c) 2014, Peter Trško <peter.trsko@gmail.com>
 * 
 * All rights reserved.
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
