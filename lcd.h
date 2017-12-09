#ifndef LCD_H
#define LCD_H
int lcd_cycle(void);
int lcd_get_line(void);
unsigned char lcd_get_stat();
void lcd_write_control(unsigned char);
void lcd_write_stat(unsigned char);
void lcd_write_scroll_x(unsigned char);
void lcd_write_scroll_y(unsigned char);
void lcd_write_bg_palette(unsigned char);
void lcd_write_spr_palette1(unsigned char);
void lcd_write_spr_palette2(unsigned char);
void lcd_set_window_y(unsigned char);
void lcd_set_window_x(unsigned char);
void lcd_set_ly_compare(unsigned char);
void lcd_set_off(int x,int y);
int lcd_get_xoff();
int lcd_get_yoff();
void lcd_gen_scale_arr(unsigned char w,unsigned char h);
#endif
