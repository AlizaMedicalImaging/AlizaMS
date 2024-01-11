#ifndef MDCM_MACRO_H
#define MDCM_MACRO_H

#define MDCM_DISALLOW_COPY_AND_MOVE(T) \
  T(const T &) = delete;               \
  T & operator=(const T &) = delete;   \
  T(T &&) = delete;                    \
  T & operator=(T &&) = delete

#endif
