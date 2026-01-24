#pragma once

#include "fifo.h"

extern volatile struct fifo8 usbdev_acm_forth_d2h_fifo;
extern volatile struct fifo8 usbdev_acm_forth_h2d_fifo;
extern volatile bool usbdev_acm_forth_h2d_disable;

extern volatile struct fifo8 usbdev_acm_data_d2h_fifo;
extern volatile struct fifo8 usbdev_acm_data_h2d_fifo;
extern volatile bool usbdev_acm_data_h2d_disable;


extern void usbdev_init(void);
