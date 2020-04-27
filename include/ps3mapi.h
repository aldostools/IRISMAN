#define SYSCALL8_OPCODE_PS3MAPI			 			0x7777
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO			0x0047
#define PS3MAPI_OPCODE_GET_CORE_MINVERSION			0x0012

LV2_SYSCALL ps3mapi_get_core_minversion(void)
{
    lv2syscall2(SYSCALL_MAMBA, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION);
    return_to_user_prog(s32);
}

static void sys8_get_plugin_slot(unsigned int slot, char *tmp_name, char *tmp_filename)
{
    memset(tmp_name, 0, sizeof(tmp_name));
    memset(tmp_filename, 0, sizeof(tmp_filename));

    lv2syscall5(SYSCALL_MAMBA, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)tmp_name, (u64)tmp_filename);
}

#define PS3MAPI_OPCODE_GET_ALL_PROC_PID				0x0021
#define PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID		0x0041
#define PS3MAPI_OPCODE_GET_PROC_MODULE_NAME			0x0042

static bool find_vsh_process(const char *name)
{
	u32 pid_list[16];
	lv2syscall3(SYSCALL_MAMBA, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)&pid_list);

	u32 pid = 0;
	for(unsigned int i = 0; i < 16; i++)
	{
		if(1 < pid_list[i]) pid = pid_list[i];
	}
	if(pid)
	{
		u32 mod_list[62];
		char tmp_name[30];
		lv2syscall4(SYSCALL_MAMBA, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID, (u64)pid, (u64)&mod_list);

		for(unsigned int slot = 0; slot <= 10; slot++)
		{
			memset(tmp_name, 0, sizeof(tmp_name));
			if(1 < mod_list[slot])
			{
				lv2syscall5(SYSCALL_MAMBA, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_NAME, (u64)&pid, (u64)&mod_list[slot], (u64)&tmp_name);

				if(!strcmp(tmp_name, name)) return true;
			}
		}
	}
	return false;
}

unsigned int get_vsh_plugin_slot_by_name(const char *name)
{
    char tmp_name[30];
    char tmp_filename[256];
    unsigned int slot;

    bool find_free_slot = (!name || (*name == 0));

    for (slot = 1; slot < 7; slot++)
    {
        sys8_get_plugin_slot(slot, tmp_name, tmp_filename);

        if(find_free_slot) {if(*tmp_name) continue; return slot;}

        if(!strcmp(tmp_name, name) || strstr(tmp_filename, name)) return slot;
    }

	if(find_vsh_process(name)) return 1;

    return 0;
}

unsigned int get_vsh_plugin_free_slot(void)
{
    if(ps3mapi_get_core_minversion() == 0) return 6;

    char tmp_name[30];
    char tmp_filename[256];
    int slot;

    for (slot = 1; slot < 7; slot++)
    {
        memset(tmp_name, 0, sizeof(tmp_name));
        memset(tmp_filename, 0, sizeof(tmp_filename));
        lv2syscall5(SYSCALL_MAMBA, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)tmp_name, (u64)tmp_filename);
        if(strlen(tmp_filename) == 0 && strlen(tmp_name) == 0) {return slot;}
    }

    return FAILED;
}
