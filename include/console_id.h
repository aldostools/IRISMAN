/*
    (c) 2011 Hermes/Estwald <www.elotrolado.net>
    IrisManager (HMANAGER port) (c) 2011 D_Skywalk <http://david.dantoine.org>

    HMANAGER4 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    HMANAGER4 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with HMANAGER4.  If not, see <http://www.gnu.org/licenses/>.

*/

void draw_console_id_tools(float x, float y);
void get_console_id_eid5();

void set_console_id_lv2();
void get_console_id_lv2();
void get_console_id_val();
int get_console_id_keyb();
int is_valid_idps();

void set_psid_lv2();
void get_psid_lv2();
void get_psid_val();
int get_psid_keyb();
bool is_valid_psid();

bool is_hex_char(char c);

int save_spoofed_psid();
int load_spoofed_psid();

int save_spoofed_console_id();
int load_spoofed_console_id();
