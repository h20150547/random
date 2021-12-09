/***************************************************************************//**
*  \file       driver.c
*
*  \details    Simple GPIO driver explanation (GPIO Interrupt)
*
*  \author     EmbeTronicX
*
*  \Tested with Linux raspberrypi 5.4.51-v7l+
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  //copy_to/from_user()
#include <linux/gpio.h>     //GPIO
#include <linux/interrupt.h>

/* Since debounce is not supported in Raspberry pi, I have addded this to disable 
** the false detection (multiple IRQ trigger for one interrupt).
** Many other hardware supports GPIO debounce, I don't want care about this even 
** if this has any overhead. Our intention is to explain the GPIO interrupt.
** If you want to disable this extra coding, you can comment the below macro.
** This has been taken from : https://raspberrypi.stackexchange.com/questions/8544/gpio-interrupt-debounce
**
** If you want to use Hardaware Debounce, then comment this EN_DEBOUNCE.
**
*/
#define EN_DEBOUNCE

#ifdef EN_DEBOUNCE
#include <linux/jiffies.h>

extern unsigned long volatile jiffies;
unsigned long old_jiffie = 0;
#endif

//LED is connected to this GPIO
#define GPIO_21_OUT (21)

//PIR1 is connected to this GPIO
#define GPIO_25_IN  (19)

//PIR1 is connected to this GPIO
#define GPIO_25_IN  (13)

//GPIO_25_IN value toggle
unsigned int led_toggle = 0; 

/* Storing GPIO IRQ Numbers */
unsigned int GPIO_irq1; // For GPIO_PIR1
unsigned int GPIO_irq2; // For GPIO_PIR2

//Interrupt handler for GPIO_PIR1
static irqreturn_t gpio_irq1_handler(int irq,void *dev_id) 
{
  static unsigned long flags = 0;
  
#ifdef EN_DEBOUNCE
   unsigned long diff = jiffies - old_jiffie;
   if (diff < 20)
   {
     return IRQ_HANDLED;
   }
  
  old_jiffie = jiffies;
#endif  

  local_irq_save(flags);
  // led_toggle = (0x01 ^ led_toggle);                             // toggle the old value
  // gpio_set_value(GPIO_21_OUT, led_toggle);                      // toggle the GPIO_21_OUT
  pr_info("Interrupt Occurred : GPIO_PIR1 : %d ",gpio_get_value(GPIO_PIR1));
  local_irq_restore(flags);
  return IRQ_HANDLED;
}

//Interrupt handler for GPIO_PIR2
static irqreturn_t gpio_irq2_handler(int irq,void *dev_id) 
{
  static unsigned long flags = 0;
  
#ifdef EN_DEBOUNCE
   unsigned long diff = jiffies - old_jiffie;
   if (diff < 20)
   {
     return IRQ_HANDLED;
   }
  
  old_jiffie = jiffies;
#endif  

  local_irq_save(flags);
  // led_toggle = (0x01 ^ led_toggle);                             // toggle the old value
  // gpio_set_value(GPIO_21_OUT, led_toggle);                      // toggle the GPIO_21_OUT
  pr_info("Interrupt Occurred : GPIO_PIR2 : %d ",gpio_get_value(GPIO_PIR2));
  local_irq_restore(flags);
  return IRQ_HANDLED;
}
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
 
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
 
 
/*************** Driver functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t etx_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
/******************************************************/

//File operation structure 
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = etx_read,
  .write          = etx_write,
  .open           = etx_open,
  .release        = etx_release,
};

static int gpio_num = GPIO_PIR1;
static int gpio_num2 = GPIO_PIR2;

/*
** This function will be called when we open the Device file
*/ 
static int etx_open(struct inode *inode, struct file *file)
{
  pr_info("Device File Opened...!!!\n");
  return 0;
}

/*
** This function will be called when we close the Device file
*/ 
static int etx_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed...!!!\n");
  return 0;
}

/*
** This function will be called when we read the Device file
*/ 
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len, loff_t *off)
{
	/* New Code  */
	uint8_t gpio_state_1 = 0;
  	uint8_t gpio_state_2 = 0;

	//reading GPIO value
	gpio_state = gpio_get_value(GPIO_PIR1);
	gpio_state = gpio_get_value(GPIO_PIR2);

	//write to user
	len = 1;
	if( copy_to_user(buf, &gpio_state_1, len) > 0) {
		pr_err("ERROR: Not all the bytes of gpio_state_1 have been copied to user\n");
	}

	if( copy_to_user(buf, &gpio_state_2, len) > 0) {
		pr_err("ERROR: Not all the bytes of of gpio_state_2 have been copied to user\n");
	}
	
	pr_info("Read function : GPIO_PIR1 = %d \n", gpio_state_1);
	pr_info("Read function : GPIO_PIR2 = %d \n", gpio_state_2);

	return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, 
                const char __user *buf, size_t len, loff_t *off)
{
  uint8_t rec_buf[10] = {0};
  
  if( copy_from_user( rec_buf, buf, len ) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
  }
  
  pr_info("Write Function : GPIO_21 Set = %c\n", rec_buf[0]);
  
  if (rec_buf[0]=='1') {
    //set the GPIO value to HIGH
    gpio_set_value(GPIO_21_OUT, 1);
  } else if (rec_buf[0]=='0') {
    //set the GPIO value to LOW
    gpio_set_value(GPIO_21_OUT, 0);
  } else {
    pr_err("Unknown command : Please provide either 1 or 0 \n");
  }
  
  return len;
}

/*
** Module Init function
*/ 
static int __init etx_driver_init(void)
{
  /*Allocating Major number*/
  if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    goto r_unreg;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

  /*Creating cdev structure*/
  cdev_init(&etx_cdev,&fops);

  /*Adding character device to the system*/
  if((cdev_add(&etx_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }

  /*Creating struct class*/
  if((dev_class = class_create(THIS_MODULE,"etx_class")) == NULL){
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }

  /*Creating device*/
  if((device_create(dev_class,NULL,dev,NULL,"etx_device")) == NULL){
    pr_err( "Cannot create the Device \n");
    goto r_device;
  }
  
  //Output GPIO configuration
  //Checking the GPIO is valid or not
  if(gpio_is_valid(GPIO_21_OUT) == false){
    pr_err("GPIO %d is not valid\n", GPIO_21_OUT);
    goto r_device;
  }
  
  //Requesting the GPIO
  if(gpio_request(GPIO_21_OUT,"GPIO_21_OUT") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_21_OUT);
    goto r_gpio_out;
  }
  
  //configure the GPIO as output
  gpio_direction_output(GPIO_21_OUT, 0);
  
 //Input GPIO configuratioin
  //Checking the GPIO is valid or not
  if(gpio_is_valid(GPIO_PIR1) == false){
    pr_err("GPIO %d is not valid\n", GPIO_PIR1);
    goto r_gpio_in;
  }
    if(gpio_is_valid(GPIO_PIR2) == false){
    pr_err("GPIO %d is not valid\n", GPIO_PIR2);
    goto r_gpio_in;
  }
  
  //Requesting the GPIO
  if(gpio_request(GPIO_PIR1,"GPIO_PIR1") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_PIR1);
    goto r_gpio_in;
  }
    if(gpio_request(GPIO_PIR2,"GPIO_PIR2") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_PIR2);
    goto r_gpio_in;
  }
  
  //configure the GPIO as input
  gpio_direction_input(GPIO_PIR1);
  gpio_direction_input(GPIO_PIR2);
  
  /*
  ** I have commented the below few lines, as gpio_set_debounce is not supported 
  ** in the Raspberry pi. So we are using EN_DEBOUNCE to handle this in this driver.
  */ 
#ifndef EN_DEBOUNCE
  //Debounce the button with a delay of 200ms
  if(gpio_set_debounce(GPIO_PIR1, 200) < 0){
    pr_err("ERROR: gpio_set_debounce - %d\n", GPIO_PIR1);
    //goto r_gpio_in;
  }
  if(gpio_set_debounce(GPIO_PIR2, 200) < 0){
    pr_err("ERROR: gpio_set_debounce - %d\n", GPIO_PIR2);
    //goto r_gpio_in;
  }  
#endif
  
  //Get the IRQ number for our GPIO
  GPIO_irq1 = gpio_to_irq(GPIO_PIR1);
  pr_info("GPIO_irqNumber = %d\n", GPIO_irqNumber);

  GPIO_irq2 = gpio_to_irq(GPIO_PIR2);
  pr_info("GPIO_irqNumber = %d\n", GPIO_irqNumber);

  if (request_irq(GPIO_irq1,             //IRQ number
                  (void *)gpio_irq1_handler,   //IRQ handler
                  IRQF_TRIGGER_RISING,        //Handler will be called in raising edge
                  "pir_device",               //used to identify the device name using this IRQ
                  NULL)) {                    //device id for shared IRQ
    pr_err("pir_device: cannot register IRQ ");
    goto r_gpio_in;

  if (request_irq(GPIO_irq2,             //IRQ number
                  (void *)gpio_irq2_handler,   //IRQ handler
                  IRQF_TRIGGER_RISING,        //Handler will be called in raising edge
                  "pir_device",               //used to identify the device name using this IRQ
                  NULL)) {                    //device id for shared IRQ
    pr_err("pir_device: cannot register IRQ ");
    goto r_gpio_in;
 
  pr_info("Device Driver Insert...Done!!!\n");
  return 0;

r_gpio_in:
  	gpio_free(GPIO_PIR1);
	gpio_free(GPIO_PIR2);
r_gpio_out:
  gpio_free(GPIO_21_OUT);
r_device:
  device_destroy(dev_class,dev);
r_class:
  class_destroy(dev_class);
r_del:
  cdev_del(&etx_cdev);
r_unreg:
  unregister_chrdev_region(dev,1);
  
  return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void)
{
  free_irq(GPIO_irq1,NULL);
  free_irq(GPIO_irq2,NULL);
  gpio_free(GPIO_PIR1);
  gpio_free(GPIO_PIR2);
  gpio_free(GPIO_21_OUT);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&etx_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Device Driver Remove...Done!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_DESCRIPTION("Smart Parking Lot System Driver for Raspberry Pi 3B");
MODULE_AUTHOR("Naren (2015HD400547P) and Nishad (2016HS400215P)");
MODULE_LICENSE("GPL");
MODULE_INFO(ChipSupport, "2 x HC-SR501");