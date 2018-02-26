#ifndef LCD_H
#define LCD_H

struct lcd_config {
	int lcd_line;
	int lcd_ly_compare;
	int ly_int;
	int lcd_mode;
	int lcd_enabled;
	int window_tilemap_select;
	int window_enabled;
	int tilemap_select;
	int bg_tiledata_select;
	int sprite_size;
	int sprites_enabled;
	int bg_enabled;
	int scroll_x, scroll_y;
	int window_x, window_y;
	int bgpalette[4];
	int sprpalette1[4];
	int sprpalette2[4];
};

void lcd_get_conf(struct lcd_config *dst);
void lcd_set_conf(struct lcd_config *src);
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
