#ifndef LCD_H
#define LCD_H
//LCD config

extern int lcd_line;
extern int lcd_ly_compare;
extern int ly_int;
extern int lcd_mode;
extern int lcd_enabled;
extern int window_tilemap_select;
extern int window_enabled;
extern int tilemap_select;
extern int bg_tiledata_select;
extern int sprite_size;
extern int sprites_enabled;
extern int bg_enabled;
extern int scroll_x, scroll_y;
extern int window_x, window_y;
extern int bgpalette[4];
extern int sprpalette1[4];
extern int sprpalette2[4];

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
