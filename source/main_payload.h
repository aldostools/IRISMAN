    if(is_firm_341())
    {
        firmware  = 0x341C;
        //fw_ver    = 0x8534;
        off_idps  = 0x80000000003BA880ULL;
        off_idps2 = 0x800000000044A174ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_341();
    }
    else if(is_firm_355())
    {
        firmware  = 0x355C;
        //fw_ver    = 0x8AAC;
        off_idps  = 0x80000000003C2EF0ULL;
        off_idps2 = 0x8000000000452174ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_355();
    }
    else if(is_firm_355dex())
    {
        firmware  = 0x355D;
        //fw_ver    = 0x8AAC;
        off_idps  = 0x80000000003DE170ULL;
        off_idps2 = 0x8000000000472174ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_355dex();
    }
    else if(is_firm_355deh())
    {
        firmware  = 0x355E;
        //fw_ver    = 0x8AAC;
        off_idps  = 0x8000000000410F70ULL;
        off_idps2 = 0x80000000004A2174ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_355deh();
    }
    else if(is_firm_421())
    {
        firmware  = 0x421C;
        //fw_ver    = 0xA474;
        off_idps  = 0x80000000003D9230ULL;
        off_idps2 = 0x8000000000477E9CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_421();
    }
    else if(is_firm_430())
    {
        firmware  = 0x430C;
        //fw_ver    = 0xA7F8;
        off_idps  = 0x80000000003DB1B0ULL;
        off_idps2 = 0x8000000000476F3CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_430();
    }
    else if(is_firm_431())
    {
        firmware  = 0x431C;
        //fw_ver    = 0xA85C;
        off_idps  = 0x80000000003DB1B0ULL;
        off_idps2 = 0x8000000000476F3CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_431();
    }
    else if(is_firm_440())
    {
        firmware  = 0x440C;
        //fw_ver    = 0xABE0;
        off_idps  = 0x80000000003DB830ULL;
        off_idps2 = 0x8000000000476F3CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_440();
    }
    else if(is_firm_441())
    {
        firmware  = 0x441C;
        //fw_ver    = 0xAC44;
        off_idps  = 0x80000000003DB830ULL;
        off_idps2 = 0x8000000000476F3CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_441();
    }
    else if(is_firm_446())
    {
        firmware  = 0x446C;
        //fw_ver    = 0xAE38;
        off_idps  = 0x80000000003DBE30ULL;
        off_idps2 = 0x8000000000476F3CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_446();
    }
    else if(is_firm_450())
    {
        firmware  = 0x450C;
        //fw_ver    = 0xAFC8;
        off_idps  = 0x80000000003DE230ULL;
        off_idps2 = 0x800000000046CF0CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_450();
    }
    else if(is_firm_453())
    {
        firmware  = 0x453C;
        //fw_ver    = 0xB0F4;
        off_idps  = 0x80000000003DE430ULL;
        off_idps2 = 0x800000000046CF0CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_453();
    }
    else if(is_firm_455())
    {
        firmware  = 0x455C;
        //fw_ver    = 0xB1BC;
        off_idps  = 0x80000000003E17B0ULL;
        off_idps2 = 0x8000000000474F1CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_455();
    }
    else if(is_firm_460())
    {
        firmware  = 0x460C;
        //fw_ver    = 0xB3B0;
        off_idps  = 0x80000000003E2BB0ULL; //thanks Orion90 for the offsets & ERMAK for the LV2 dump of 4.60
        off_idps2 = 0x8000000000474F1CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_460();
    }
    else if(is_firm_465())
    {
        firmware  = 0x465C;
        //fw_ver    = 0xB5A4;
        off_idps  = 0x80000000003E2BB0ULL; // The same as 4.60 CEX
        off_idps2 = 0x8000000000474F1CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_465();
    }
    else if(is_firm_466())
    {
        firmware  = 0x466C;
        //fw_ver    = 0xB608;
        off_idps  = 0x80000000003E2BB0ULL; // The same as 4.60-4.65 CEX
        off_idps2 = 0x8000000000474F1CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_465();
    }
    else if(is_firm_470())
    {
        firmware  = 0x470C;
        //fw_ver    = 0xB798;
        off_idps  = 0x80000000003E2DB0ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_470();
    }
    else if(is_firm_475())
    {
        firmware  = 0x475C;
        //fw_ver    = 0xB98C;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_476())
    {
        firmware  = 0x476C;
        //fw_ver    = 0xB9F0;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_466dex())
    {
        firmware  = 0x466D;
        //fw_ver    = 0xB608;
        off_idps  = 0x80000000004095B0ULL; // The same as 4.60-4.65 CEX
        off_idps2 = 0x800000000049CF1CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_465dex();
    }
    else if(is_firm_478())
    {
        firmware  = 0x478C;
        //fw_ver    = 0xBAB8;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_480())
    {
        firmware  = 0x480C;
        //fw_ver    = 0xBB80;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_480();
    }
    else if(is_firm_481())
    {
        firmware  = 0x481C;
        //fw_ver    = 0xBBE4;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_482())
    {
        firmware  = 0x482C;
        //fw_ver    = 0xBC48;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_483())
    {
        firmware  = 0x483C;
        //fw_ver    = 0xBCAC;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_484())
    {
        firmware  = 0x484C;
        //fw_ver    = 0xBD10;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_485())
    {
        firmware  = 0x485C;
        //fw_ver    = 0xBD74;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_486())
    {
        firmware  = 0x486C;
        //fw_ver    = 0xBDD8;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_48X())
    {
        firmware  = 0x487C;
        //fw_ver    = 0xBE3C;
        off_idps  = 0x80000000003E2E30ULL;
        off_idps2 = 0x8000000000474AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475();
    }
    else if(is_firm_421dex())
    {
        firmware  = 0x421D;
        //fw_ver    = 0xA474;
        off_idps  = 0x80000000003F7A30ULL;
        off_idps2 = 0x800000000048FE9CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_421dex();
    }
    else if(is_firm_430dex())
    {
        firmware  = 0x430D;
        //fw_ver    = 0xA7F8;
        off_idps  = 0x80000000003F9930ULL;
        off_idps2 = 0x8000000000496F3CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_430dex();
    }
    else if(is_firm_441dex())
    {
        //if( file_exists( "/dev_flash/ps3itald.self" ) && file_exists( "/dev_flash/ps3ita" ) ) is_ps3ita = true;
        firmware  = 0x441D;
        //fw_ver    = 0xAC44;
        off_idps  = 0x80000000003FA2B0ULL;
        off_idps2 = 0x8000000000496F3CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_441dex();
    }
    else if(is_firm_446dex())
    {
        firmware  = 0x446D;
        //fw_ver    = 0xAE38;
        off_idps  = 0x80000000003FA8B0ULL;
        off_idps2 = 0x8000000000496F3CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_446dex();
    }
    else if(is_firm_450dex())
    {
        //if( file_exists( "/dev_flash/ps3itald.self" ) && file_exists( "/dev_flash/ps3ita" ) ) is_ps3ita = true;
        firmware  = 0x450D;
        //fw_ver    = 0xAFC8;
        off_idps  = 0x8000000000402AB0ULL;
        off_idps2 = 0x8000000000494F0CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_450dex();
    }
    else if(is_firm_453dex())
    {
        firmware  = 0x453D;
        //fw_ver    = 0xB0F4;
        off_idps  = 0x80000000004045B0ULL;
        off_idps2 = 0x8000000000494F1CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_453dex();
    }
    else if(is_firm_455dex())
    {
        //if(file_exists( "/dev_flash/ps3ita" )) is_ps3ita = true;
        firmware  = 0x455D;
        //fw_ver    = 0xB1BC;
        off_idps  = 0x8000000000407930ULL;
        off_idps2 = 0x8000000000494F1CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_455dex();
    }
    else if(is_firm_460dex())
    {
        firmware  = 0x460D;
        //fw_ver    = 0xB3B0;
        off_idps  = 0x80000000004095B0ULL;
        off_idps2 = 0x800000000049CF1CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_460dex();
    }
    else if(is_firm_465dex())
    {
        firmware  = 0x465D;
        //fw_ver    = 0xB5A4;
        off_idps  = 0x80000000004095B0ULL;
        off_idps2 = 0x800000000049CF1CULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_465dex();
    }
    else if(is_firm_470dex())
    {
        firmware  = 0x470D;
        //fw_ver    = 0xB798;
        off_idps  = 0x80000000004098B0ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_470dex();
    }
    else if(is_firm_475dex())
    {
        firmware  = 0x475D;
        //fw_ver    = 0xB98C;
        off_idps  = 0x8000000000409930ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475dex();
    }
    else if(is_firm_476dex())
    {
        firmware  = 0x476D;
        //fw_ver    = 0B9F0;
        off_idps  = 0x8000000000409930ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475dex();
    }
    else if(is_firm_478dex())
    {
        firmware  = 0x478D;
        //fw_ver    = 0xBAB8;
        off_idps  = 0x8000000000409930ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475dex();
    }
    else if(is_firm_481dex())
    {
        firmware  = 0x481D;
        //fw_ver    = 0xBBE4;
        off_idps  = 0x8000000000409A30ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_481dex();
    }
    else if(is_firm_482dex())
    {
        firmware  = 0x482D;
        //fw_ver    = 0xBC48;
        off_idps  = 0x8000000000409A30ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_481dex();
    }
    else if(is_firm_483dex())
    {
        firmware  = 0x483D;
        //fw_ver    = 0xBCAC;
        off_idps  = 0x8000000000409A30ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_481dex();
    }
    else if(is_firm_484dex())
    {
        firmware  = 0x484D;
        //fw_ver    = 0xBD10;
        off_idps  = 0x8000000000409A30ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_481dex();
    }
    else if(is_firm_485dex())
    {
        firmware  = 0x485D;
        //fw_ver    = 0xBD74;
        off_idps  = 0x8000000000409A30ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_481dex();
    }
    else if(is_firm_486dex())
    {
        firmware  = 0x486D;
        //fw_ver    = 0xBDD8;
        off_idps  = 0x8000000000409A30ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_481dex();
    }
    else if(is_firm_48Xdex())
    {
        firmware  = 0x487D;
        //fw_ver    = 0xBE3C;
        off_idps  = 0x8000000000409A30ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_481dex();
    }
    else if(is_firm_460deh())
    {
        firmware  = 0x460E;
        //fw_ver    = 0xB798;
        off_idps  = 0x8000000000432430ULL;
        off_idps2 = 0x80000000004C4F18ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_460deh();
    }
    else if(is_firm_475deh())
    {
        firmware  = 0x475E;
        //fw_ver    = 0xB98C;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }
    else if(is_firm_476deh())
    {
        firmware  = 0x476E;
        //fw_ver    = 0B9F0;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }
    else if(is_firm_478deh())
    {
        firmware  = 0x478E;
        //fw_ver    = 0xBAB8;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }
    else if(is_firm_480dex())
    {
        firmware  = 0x480D;
        //fw_ver    = 0xBB80;
        off_idps  = 0x8000000000409A30ULL;
        off_idps2 = 0x800000000049CAF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_480dex();
    }
    else if(is_firm_480deh())
    {
        firmware  = 0x480E;
        //fw_ver    = 0xBB80;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_480deh();
    }
    else if(is_firm_481deh())
    {
        firmware  = 0x481E;
        //fw_ver    = 0xBBE4;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }
    else if(is_firm_482deh())
    {
        firmware  = 0x482E;
        //fw_ver    = 0xBc48;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }
    else if(is_firm_483deh())
    {
        firmware  = 0x483E;
        //fw_ver    = 0xBCAC;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }
    else if(is_firm_484deh())
    {
        firmware  = 0x484E;
        //fw_ver    = 0xBD10;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }
    else if(is_firm_485deh())
    {
        firmware  = 0x485E;
        //fw_ver    = 0xBD74;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }
    else if(is_firm_486deh())
    {
        firmware  = 0x486E;
        //fw_ver    = 0xBDD8;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }
    else if(is_firm_48Xdeh())
    {
        firmware  = 0x487E;
        //fw_ver    = 0xBE3C;
        off_idps  = 0x80000000004326B0ULL;
        off_idps2 = 0x80000000004C4AF4ULL;
        off_psid  = off_idps2 + 0x18ULL;
        payload_mode = is_payload_loaded_475deh();
    }

    if(is_cobra_based()) use_cobra = true;

    if(lv1peek(0x1773ULL) == 0x1773ULL)
    {
        unsupported_cfw = is_ps3hen = true;
        payload_mode = PAYLOAD_PS3HEN; // HENtai payload
    }


    //sprintf(temp_buffer + 0x1000, "firmware: %xex payload %i", firmware, payload_mode);


    // read xRegistry data
    read_from_registry();

    // read custom settings
    read_settings();

    ///////////////////////////
    *payload_str = 0;
    if(is_ps3hen) goto payload_ps3hen;

    switch(firmware)
    {
        case 0x341C:
            set_bdvdemu_341(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_341(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case HERMES_PAYLOAD:
                    break;
            }
            break;
        case 0x355C:
            set_bdvdemu_355(payload_mode);
            switch(payload_mode)
            {
                case WANIN_PAYLOAD:
                case ZERO_PAYLOAD: //no payload installed
                    install_new_poke(); /* need for patch lv2 */

                    if (!map_lv1())
                    {
                        remove_new_poke();

                        tiny3d_Init(1024*1024);
                        ioPadInit(7);
                        DrawDialogOK("Error Loading Payload: map failed?!");
                        exit(0);
                    }

                    patch_lv2_protection(); /* yaw */
                    remove_new_poke(); /* restore pokes */

                    unmap_lv1();  /* 3.55 need unmap? */

                    __asm__("sync");
                    sleep(1); /* dont touch! nein! */

                    //please, do not translate this strings - i preffer this errors in english for better support...
                    if(payload_mode == WANIN_PAYLOAD)
                    {
                        sys8_disable_all = 1;
                        sprintf(temp_buffer, "WANINV2 DETECTED\nOLD SYSCALL 36 LOADED (mode=%i)\n\n - no big files allowed with this payload -", payload_mode);
                        sprintf(payload_str, "wanin cfw - old syscall36, no bigfiles allowed");
                    }
                    else
                    {
                        load_payload_355(payload_mode);

                        __asm__("sync");
                        sleep(1); /* maybe need it, maybe not */

                        if(!use_cobra && install_mamba)
                        {
                            use_mamba = load_ps3_mamba_payload();
                        }
                    }
                    break;
                case SYS36_PAYLOAD:
                    sys8_disable_all = 1;
                    sprintf(temp_buffer, "OLD SYSCALL 36 RESIDENT, RESPECT!\nNEW PAYLOAD NOT LOADED...\n\n - no big files allowed with this payload -");
                    sprintf(payload_str, "syscall36 resident - new payload no loaded, no bigfiles allowed");
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x355D: //355dex
            set_bdvdemu_355dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    install_new_poke_355dex();
                    if (!map_lv1_355dex())
                    {
                        remove_new_poke_355dex();

                        tiny3d_Init(1024*1024);
                        ioPadInit(7);
                        DrawDialogOK("Error Loading Payload: map failed?!");
                        exit(0);
                    }
                    patch_lv2_protection_355dex(); /* yaw */

                    remove_new_poke_355dex(); /* restore pokes */
                    unmap_lv1_355dex();  /* 3.55 need unmap? */
                    __asm__("sync");

                    load_payload_355dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;

        case 0x355E: //355deh
            set_bdvdemu_355deh(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    install_new_poke_355deh();
                    if (!map_lv1_355deh())
                    {
                        remove_new_poke_355deh();

                        tiny3d_Init(1024*1024);
                        ioPadInit(7);
                        DrawDialogOK("Error Loading Payload: map failed?!");
                        exit(0);
                    }
                    patch_lv2_protection_355deh(); /* yaw */

                    remove_new_poke_355deh(); /* restore pokes */
                    unmap_lv1_355deh();  /* 3.55 need unmap? */
                    __asm__("sync");

                    load_payload_355deh(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;

        case 0x421C:
            set_bdvdemu_421(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_421(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x421D: //4.21 dex
            set_bdvdemu_421dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_421dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x430C:
            set_bdvdemu_430(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_430(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x430D:
            set_bdvdemu_430dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_430dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x431C:
            set_bdvdemu_431(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_431(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x440C:
            set_bdvdemu_440(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_440(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x441C:
            set_bdvdemu_441(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_441(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x441D:
            set_bdvdemu_441dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_441dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x446C:
            set_bdvdemu_446(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_446(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x446D:
            set_bdvdemu_446dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_446dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x450C:
            set_bdvdemu_450(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_450(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x450D:
            set_bdvdemu_450dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_450dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x453C:
            set_bdvdemu_453(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_453(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x453D:
            set_bdvdemu_453dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_453dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x455C:
            set_bdvdemu_455(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_455(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x455D:
            set_bdvdemu_455dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_455dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x460C:
            set_bdvdemu_460(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_460(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x460D:
            set_bdvdemu_460dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_460dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x460E:
            set_bdvdemu_460dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_460dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x465C:
        case 0x466C:
            set_bdvdemu_465(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_465(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x465D:
        case 0x466D:
            set_bdvdemu_465dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_465dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x470C:
            set_bdvdemu_470(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_470(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x470D:
            set_bdvdemu_470dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_470dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x475C:
        case 0x476C:
        case 0x478C:
        case 0x481C:
        case 0x482C:
        case 0x483C:
        case 0x484C:
        case 0x485C:
        case 0x486C:
        case 0x487C:
            set_bdvdemu_475(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_475(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x475D:
        case 0x476D:
        case 0x478D:
            set_bdvdemu_475dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_475dex(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x475E:
        case 0x476E:
        case 0x478E:
        case 0x481E:
        case 0x482E:
        case 0x483E:
        case 0x484E:
        case 0x485E:
        case 0x486E:
        case 0x487E:
            set_bdvdemu_475deh(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_475deh(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x480C:
            set_bdvdemu_480(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_480(payload_mode);
                    __asm__("sync");
                    sleep(1); /* maybe need it, maybe not */

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x480D:
            set_bdvdemu_480dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_480dex(payload_mode);
                    __asm__("sync");
                    sleep(1); // maybe need it, maybe not

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x480E:
            set_bdvdemu_480deh(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_480deh(payload_mode);
                    __asm__("sync");
                    sleep(1); // maybe need it, maybe not

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        case 0x481D:
        case 0x482D:
        case 0x483D:
        case 0x484D:
        case 0x485D:
        case 0x486D:
        case 0x487D:
            set_bdvdemu_481dex(payload_mode);
            switch(payload_mode)
            {
                case ZERO_PAYLOAD: //no payload installed
                    load_payload_481dex(payload_mode);
                    __asm__("sync");
                    sleep(1); // maybe need it, maybe not

                    if(!use_cobra && install_mamba)
                    {
                        use_mamba = load_ps3_mamba_payload();
                    }
                    break;
                case SKY10_PAYLOAD:
                    break;
            }
            break;
        default:
            tiny3d_Init(1024*1024);
            ioPadInit(7);
            //DrawDialogOK("ERROR: Unsupported firmware or the syscalls are disabled!\n\nSome functions will be limited.");
            unsupported_cfw = true;
            break;
    }

    if(use_mamba && !use_cobra)
    {
        syscall_40(1, 0); // disables PS3 Disc-less / load mamba
        if(is_cobra_based()) use_cobra = true;
    }

    usleep(250000);


    if(payload_mode >= ZERO_PAYLOAD && sys8_disable_all == 0)
    {
        int test = 0x100;

        uint16_t mamba_version = 0;
        if(use_cobra) sys8_mamba_version(&mamba_version);
        if(use_mamba) is_mamba_v2 = (mamba_version < 0x0801);

        //check syscall8 status
        test = sys8_enable(0ULL);
        if((test & 0xff00) == 0x300)
        {
            if(payload_mode == ZERO_PAYLOAD)
            {
                if(firmware== 0x341C)
                        sprintf(payload_str, "payload-hermes - new syscall%i v%i (%slibfs_patched)", (u16)SYSCALL_SK1E, test & 0xff, is_libfs_patched()? "": "no ");
                else
                {
                    if(use_mamba)
                        sprintf(payload_str, "payload-sk1e sc%i - '%s v%X' syscall8 v%i (%slibfs_patched)", (u16)SYSCALL_SK1E, is_mamba_v2 ? "Mamba 2.x" : "Mamba 3.x", mamba_version, test & 0xff, is_libfs_patched()? "": "no ");
                    else if(use_cobra)
                        sprintf(payload_str, "payload-sk1e sc%i - 'Cobra v%X' syscall8 v%i (%slibfs_patched)", (u16)SYSCALL_SK1E, mamba_version, test & 0xff, is_libfs_patched()? "": "no ");
                    else
                        sprintf(payload_str, "payload-sk1e - new syscall%i v%i (%slibfs_patched)", (u16)SYSCALL_SK1E, test & 0xff, is_libfs_patched()? "": "no ");
                }
            }
            else if (payload_mode == SKY10_PAYLOAD)
            {
                if(use_cobra && sys8_mamba() == 0x666)
                    sprintf(payload_str, "payload-sk1e sc%i - '%s v%X' syscall8 v%i (%slibfs_patched)", (u16)SYSCALL_SK1E, is_mamba_v2 ? "Mamba 2.x" : "Mamba 3.x", mamba_version, test & 0xff, is_libfs_patched()? "": "no ");
                else if(use_cobra)
                    sprintf(payload_str, "payload-sk1e sc%i - 'Cobra v%X' syscall8 v%i (%slibfs_patched)", (u16)SYSCALL_SK1E, mamba_version, test & 0xff, is_libfs_patched()? "": "no ");
                else
                    sprintf(payload_str, "payload-sk1e - new syscall%i v%i (%slibfs_patched)", (u16)SYSCALL_SK1E, test & 0xff, is_libfs_patched()? "": "no ");
            }
            else
                    sprintf(payload_str, "payload-hermes resident - new syscall%i v%i (%slibfs_patched)", (u16)SYSCALL_SK1E, test & 0xff, is_libfs_patched()? "": "no ");

        }
        else
        {
                //sprintf(payload_str, "payload-sk1e - new syscall%i Err?! v(%i)", (u16)SYSCALL_SK1E, test);
payload_ps3hen:
                //sys8_disable_all = 1;

                //if(lv1peek(0x1773ULL)==0x1773ULL)
                {
                    unsupported_cfw = false, is_ps3hen = true;
                    sprintf(payload_str, "payload-PS3HEN");
                    use_cobra = use_mamba = true; is_mamba_v2 = true;
                }
				//else if(*payload_str == 0)
				//	sprintf(payload_str, "payload-unknown");
        }
    }
