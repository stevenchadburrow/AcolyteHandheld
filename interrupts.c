

volatile void update_usb()
{
	usbhost_device_delay = usbhost_device_delay + 1;
	
	if (usbhost_device_delay >= 48) // makes it about each 1 ms
	{
		usbhost_device_delay = 0;
		usbhost_device_millis++;
	}
}

volatile void update_controllers()
{
	if (controller_enable > 0)
	{
		if (controller_config == 1) // one player
		{
			controller_status_2 = 0x00;
			controller_status_3 = 0x00;
			controller_status_4 = 0x00;
			
			if (usb_mode == 0x02) // xbox 360 controller
			{
				if (usb_readpos != usb_writepos)
				{
					controller_status_1 = 0x00;
					
					screen_speed_dir = 1;
					
					while (usb_readpos != usb_writepos)
					{
						if ((usb_buttons[usb_readpos] & 0x0100) == 0x0100) controller_status_1 = (controller_status_1 | 0x10); // up
						if ((usb_buttons[usb_readpos] & 0x0200) == 0x0200) controller_status_1 = (controller_status_1 | 0x20); // down
						if ((usb_buttons[usb_readpos] & 0x0400) == 0x0400) controller_status_1 = (controller_status_1 | 0x40); // left
						if ((usb_buttons[usb_readpos] & 0x0800) == 0x0800) controller_status_1 = (controller_status_1 | 0x80); // right
						if ((usb_buttons[usb_readpos] & 0x2000) == 0x2000) controller_status_1 = (controller_status_1 | 0x04); // select
						if ((usb_buttons[usb_readpos] & 0x1000) == 0x1000) controller_status_1 = (controller_status_1 | 0x08); // start
						//if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) controller_status_1 = (controller_status_1 | 0x01); // X
						//if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) controller_status_1 = (controller_status_1 | 0x02); // Y
						//if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) controller_status_1 = (controller_status_1 | 0x02); // A
						//if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) controller_status_1 = (controller_status_1 | 0x01); // B
						
						if (controller_mapping == 1) // changed
						{
							if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) controller_status_1 = (controller_status_1 | 0x02); // X
							if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) controller_status_1 = (controller_status_1 | 0x01); // A
							
							if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) screen_speed_dir = 2;
							if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) screen_speed_dir = 0;
						}
						else if (controller_mapping == 2) // changed
						{
							if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) controller_status_1 = (controller_status_1 | 0x02); // Y
							if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) controller_status_1 = (controller_status_1 | 0x01); // B
							
							if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) screen_speed_dir = 2;
							if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) screen_speed_dir = 0;
						}
						else // default
						{
							if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) controller_status_1 = (controller_status_1 | 0x02); // A
							if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) controller_status_1 = (controller_status_1 | 0x01); // B
							
							if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) screen_speed_dir = 2;
							if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) screen_speed_dir = 0;	
						}

						usb_readpos++;
					}
				}
			}
			else
			{
				screen_speed_dir = 1;
				
				if (controller_mapping == 0) // default Genesis controllers
				{
					if (controller_mode > 0)
					{
						controller_status_1 = (controller_status_1 & 0x0C);

						if (controller_detect_1 > 0)
						{
							controller_status_1 = controller_status_1 | 
								((!PORTKbits.RK0) << 4) | // up
								((!PORTKbits.RK1) << 5) | // down
								((!PORTKbits.RK2) << 6) | // left
								((!PORTKbits.RK3) << 7) | // right
								((!PORTKbits.RK4) << 1) | // B
								((!PORTKbits.RK5)); // A
						}

						if (controller_detect_2 > 0)
						{
							controller_status_1 = controller_status_1 | 
								((!PORTFbits.RF0) << 4) | // up
								((!PORTFbits.RF1) << 5) | // down
								((!PORTFbits.RF2) << 6) | // left
								((!PORTFbits.RF4) << 7) | // right
								((!PORTFbits.RF5) << 1) | // B
								((!PORTFbits.RF8)); // A
						}
					}
					else
					{
						if ((!PORTKbits.RK2) && (!PORTKbits.RK3) && PORTKbits.RK0 && PORTKbits.RK1)
						{
							controller_detect_1 = 0x01;
						}

						if ((!PORTFbits.RF2) && (!PORTFbits.RF4) && PORTFbits.RF0 && PORTFbits.RF1)
						{
							controller_detect_2 = 0x01;
						}

						controller_status_1 = (controller_status_1 & 0xF3);

						if (controller_detect_1 > 0)
						{
							controller_status_1 = controller_status_1 |
								((!PORTKbits.RK4) << 2) | // select
								((!PORTKbits.RK5) << 3); // start
						}

						if (controller_detect_2 > 0)
						{
							controller_status_1 = controller_status_1 |
								((!PORTFbits.RF5) << 2) | // select
								((!PORTFbits.RF8) << 3); // start
						}
					}
				}
				else // GameTank controller layout
				{
					if (controller_mode > 0)
					{
						controller_status_1 = (controller_status_1 & 0x0C);

						if (controller_detect_1 > 0)
						{
							controller_status_1 = controller_status_1 | 
								((!PORTKbits.RK0) << 4) | // up
								((!PORTKbits.RK1) << 5) | // down
								((!PORTKbits.RK2) << 6) | // left
								((!PORTKbits.RK3) << 7) | // right
								((!PORTKbits.RK4)) | // A
								((!PORTKbits.RK5) << 2); // select
						}

						if (controller_detect_2 > 0)
						{
							controller_status_1 = controller_status_1 | 
								((!PORTFbits.RF0) << 4) | // up
								((!PORTFbits.RF1) << 5) | // down
								((!PORTFbits.RF2) << 6) | // left
								((!PORTFbits.RF4) << 7) | // right
								((!PORTFbits.RF5)) | // A
								((!PORTFbits.RF8) << 2); // select
						}
					}
					else
					{
						if ((!PORTKbits.RK2) && (!PORTKbits.RK3) && PORTKbits.RK0 && PORTKbits.RK1)
						{
							controller_detect_1 = 0x01;
						}

						if ((!PORTFbits.RF2) && (!PORTFbits.RF4) && PORTFbits.RF0 && PORTFbits.RF1)
						{
							controller_detect_2 = 0x01;
						}

						controller_status_1 = (controller_status_1 & 0xF3);

						if (controller_detect_1 > 0)
						{
							controller_status_1 = controller_status_1 |
								((!PORTKbits.RK4) << 1) | // B
								((!PORTKbits.RK5) << 3); // start
						}

						if (controller_detect_2 > 0)
						{
							controller_status_1 = controller_status_1 |
								((!PORTFbits.RF5) << 1) | // B
								((!PORTFbits.RF8) << 3); // start
						}
					}
				}

				if (controller_mapping == 1) // changed
				{
					controller_status_1 = controller_status_1 | 
						((!PORTBbits.RB8) << 4) | // up
						((!PORTBbits.RB9) << 5) | // down
						((!PORTBbits.RB10) << 6) | // left
						((!PORTBbits.RB11) << 7) | // right
						((!PORTDbits.RD14) << 2) | // select
						((!PORTDbits.RD15) << 3) | // start
						((!PORTBbits.RB15) << 1) | // B
						((!PORTBbits.RB13)); // A
					
					if (PORTBbits.RB14 == 0) screen_speed_dir = 2;
					if (PORTBbits.RB12 == 0) screen_speed_dir = 0;
				}
				else if (controller_mapping == 2) // changed
				{
					controller_status_1 = controller_status_1 | 
						((!PORTBbits.RB8) << 4) | // up
						((!PORTBbits.RB9) << 5) | // down
						((!PORTBbits.RB10) << 6) | // left
						((!PORTBbits.RB11) << 7) | // right
						((!PORTDbits.RD14) << 2) | // select
						((!PORTDbits.RD15) << 3) | // start
						((!PORTBbits.RB14) << 1) | // B
						((!PORTBbits.RB12)); // A
					
					if (PORTBbits.RB15 == 0) screen_speed_dir = 2;
					if (PORTBbits.RB13 == 0) screen_speed_dir = 0;	
				}
				else // default
				{
					controller_status_1 = controller_status_1 | 
						((!PORTBbits.RB8) << 4) | // up
						((!PORTBbits.RB9) << 5) | // down
						((!PORTBbits.RB10) << 6) | // left
						((!PORTBbits.RB11) << 7) | // right
						((!PORTDbits.RD14) << 2) | // select
						((!PORTDbits.RD15) << 3) | // start
						((!PORTBbits.RB13) << 1) | // B
						((!PORTBbits.RB12)); // A
						//((!PORTBbits.RB14) << 1) | // X
						//((!PORTBbits.RB15)); // Y

					if (PORTBbits.RB14 == 0) screen_speed_dir = 2;
					if (PORTBbits.RB15 == 0) screen_speed_dir = 0;	
				}
			}
		}
		else if (controller_config == 2) // two players
		{
			controller_status_3 = 0x00;
			controller_status_4 = 0x00;
			
			if (usb_mode == 0x02) // xbox 360 controller
			{
				if (usb_readpos != usb_writepos)
				{
					controller_status_2 = 0x00;
					
					while (usb_readpos != usb_writepos)
					{
						if ((usb_buttons[usb_readpos] & 0x0100) == 0x0100) controller_status_2 = (controller_status_2 | 0x10); // up
						if ((usb_buttons[usb_readpos] & 0x0200) == 0x0200) controller_status_2 = (controller_status_2 | 0x20); // down
						if ((usb_buttons[usb_readpos] & 0x0400) == 0x0400) controller_status_2 = (controller_status_2 | 0x40); // left
						if ((usb_buttons[usb_readpos] & 0x0800) == 0x0800) controller_status_2 = (controller_status_2 | 0x80); // right
						if ((usb_buttons[usb_readpos] & 0x2000) == 0x2000) controller_status_2 = (controller_status_2 | 0x04); // select
						if ((usb_buttons[usb_readpos] & 0x1000) == 0x1000) controller_status_2 = (controller_status_2 | 0x08); // start
						//if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) controller_status_2 = (controller_status_2 | 0x01); // X
						//if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) controller_status_2 = (controller_status_2 | 0x02); // Y
						//if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) controller_status_2 = (controller_status_2 | 0x02); // A
						//if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) controller_status_2 = (controller_status_2 | 0x01); // B
						
						if (controller_mapping == 1) // changed
						{
							if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) controller_status_2 = (controller_status_2 | 0x02); // X
							if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) controller_status_2 = (controller_status_2 | 0x01); // A
							
							if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) screen_speed_dir = 2;
							if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) screen_speed_dir = 0;
						}
						else if (controller_mapping == 2) // changed
						{
							if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) controller_status_2 = (controller_status_2 | 0x02); // Y
							if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) controller_status_2 = (controller_status_2 | 0x01); // B
							
							if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) screen_speed_dir = 2;
							if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) screen_speed_dir = 0;
						}
						else // default
						{
							if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) controller_status_2 = (controller_status_2 | 0x02); // A
							if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) controller_status_2 = (controller_status_2 | 0x01); // B
							
							if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) screen_speed_dir = 2;
							if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) screen_speed_dir = 0;
						}

						usb_readpos++;
					}
				}
			}
			else
			{
				screen_speed_dir = 1;
				
				if (controller_mapping == 0) // default Genesis controllers
				{
					if (controller_mode > 0)
					{
						controller_status_2 = (controller_status_2 & 0x0C);

						if (controller_detect_2 > 0)
						{
							controller_status_2 = controller_status_2 | 
								((!PORTFbits.RF0) << 4) | // up
								((!PORTFbits.RF1) << 5) | // down
								((!PORTFbits.RF2) << 6) | // left
								((!PORTFbits.RF4) << 7) | // right
								((!PORTFbits.RF5) << 1) | // B
								((!PORTFbits.RF8)); // A
						}
					}
					else
					{
						if ((!PORTFbits.RF2) && (!PORTFbits.RF4) && PORTFbits.RF0 && PORTFbits.RF1)
						{
							controller_detect_2 = 0x01;
						}

						if (controller_detect_2 > 0)
						{
							controller_status_2 = (controller_status_2 & 0xF3);

							controller_status_2 = controller_status_2 |
								((!PORTFbits.RF5) << 2) | // select
								((!PORTFbits.RF8) << 3); // start
						}
					}
				}
				else // GameTank controller layout
				{
					if (controller_mode > 0)
					{
						controller_status_2 = (controller_status_2 & 0x0C);

						if (controller_detect_2 > 0)
						{
							controller_status_2 = controller_status_2 | 
								((!PORTFbits.RF0) << 4) | // up
								((!PORTFbits.RF1) << 5) | // down
								((!PORTFbits.RF2) << 6) | // left
								((!PORTFbits.RF4) << 7) | // right
								((!PORTFbits.RF5)) | // A
								((!PORTFbits.RF8) << 2); // select
						}
					}
					else
					{
						if ((!PORTFbits.RF2) && (!PORTFbits.RF4) && PORTFbits.RF0 && PORTFbits.RF1)
						{
							controller_detect_2 = 0x01;
						}

						if (controller_detect_2 > 0)
						{
							controller_status_2 = (controller_status_2 & 0xF3);

							controller_status_2 = controller_status_2 |
								((!PORTFbits.RF5) << 1) | // B
								((!PORTFbits.RF8) << 3); // start
						}
					}
				}
			}
			
			if (controller_mapping == 0) // default Genesis controller
			{
				if (controller_mode > 0)
				{
					controller_status_1 = (controller_status_1 & 0x0C);

					if (controller_detect_1 > 0)
					{
						controller_status_1 = controller_status_1 | 
							((!PORTKbits.RK0) << 4) | // up
							((!PORTKbits.RK1) << 5) | // down
							((!PORTKbits.RK2) << 6) | // left
							((!PORTKbits.RK3) << 7) | // right
							((!PORTKbits.RK4) << 1) | // B
							((!PORTKbits.RK5)); // A
					}
				}
				else
				{
					if ((!PORTKbits.RK2) && (!PORTKbits.RK3) && PORTKbits.RK0 && PORTKbits.RK1)
					{
						controller_detect_1 = 0x01;
					}

					controller_status_1 = (controller_status_1 & 0xF3);

					if (controller_detect_1 > 0)
					{
						controller_status_1 = controller_status_1 |
							((!PORTKbits.RK4) << 2) | // select
							((!PORTKbits.RK5) << 3); // start
					}
				}
			}
			else // GameTank controller layout
			{
				if (controller_mode > 0)
				{
					controller_status_1 = (controller_status_1 & 0x0C);

					if (controller_detect_1 > 0)
					{
						controller_status_1 = controller_status_1 | 
							((!PORTKbits.RK0) << 4) | // up
							((!PORTKbits.RK1) << 5) | // down
							((!PORTKbits.RK2) << 6) | // left
							((!PORTKbits.RK3) << 7) | // right
							((!PORTKbits.RK4)) | // A
							((!PORTKbits.RK5) << 2); // select
					}
				}
				else
				{
					if ((!PORTKbits.RK2) && (!PORTKbits.RK3) && PORTKbits.RK0 && PORTKbits.RK1)
					{
						controller_detect_1 = 0x01;
					}

					controller_status_1 = (controller_status_1 & 0xF3);

					if (controller_detect_1 > 0)
					{
						controller_status_1 = controller_status_1 |
							((!PORTKbits.RK4) << 1) | // B
							((!PORTKbits.RK5) << 3); // start
					}
				}
			}

			if (controller_mapping == 1) // changed
			{
				controller_status_1 = controller_status_1 | 
					((!PORTBbits.RB8) << 4) | // up
					((!PORTBbits.RB9) << 5) | // down
					((!PORTBbits.RB10) << 6) | // left
					((!PORTBbits.RB11) << 7) | // right
					((!PORTDbits.RD14) << 2) | // select
					((!PORTDbits.RD15) << 3) | // start
					((!PORTBbits.RB15) << 1) | // B
					((!PORTBbits.RB13)); // A
				
				if (PORTBbits.RB14 == 0) screen_speed_dir = 2;
				if (PORTBbits.RB12 == 0) screen_speed_dir = 0;
			}
			else if (controller_mapping == 2) // changed
			{
				controller_status_1 = controller_status_1 | 
					((!PORTBbits.RB8) << 4) | // up
					((!PORTBbits.RB9) << 5) | // down
					((!PORTBbits.RB10) << 6) | // left
					((!PORTBbits.RB11) << 7) | // right
					((!PORTDbits.RD14) << 2) | // select
					((!PORTDbits.RD15) << 3) | // start
					((!PORTBbits.RB14) << 1) | // B
					((!PORTBbits.RB12)); // A
				
				if (PORTBbits.RB15 == 0) screen_speed_dir = 2;
				if (PORTBbits.RB13 == 0) screen_speed_dir = 0;		
			}
			else // default
			{
				controller_status_1 = controller_status_1 | 
					((!PORTBbits.RB8) << 4) | // up
					((!PORTBbits.RB9) << 5) | // down
					((!PORTBbits.RB10) << 6) | // left
					((!PORTBbits.RB11) << 7) | // right
					((!PORTDbits.RD14) << 2) | // select
					((!PORTDbits.RD15) << 3) | // start
					((!PORTBbits.RB13) << 1) | // B
					((!PORTBbits.RB12)); // A
					//((!PORTBbits.RB14) << 1) | // X
					//((!PORTBbits.RB15)); // Y
				
				if (PORTBbits.RB14 == 0) screen_speed_dir = 2;
				if (PORTBbits.RB15 == 0) screen_speed_dir = 0;
			}
		}
		else if (controller_config == 4) // four players
		{
			if (usb_mode == 0x02) // xbox 360 controller
			{
				if (usb_readpos != usb_writepos)
				{
					controller_status_4 = 0x00;
					
					while (usb_readpos != usb_writepos)
					{						
						if ((usb_buttons[usb_readpos] & 0x0100) == 0x0100) controller_status_4 = (controller_status_4 | 0x10); // up
						if ((usb_buttons[usb_readpos] & 0x0200) == 0x0200) controller_status_4 = (controller_status_4 | 0x20); // down
						if ((usb_buttons[usb_readpos] & 0x0400) == 0x0400) controller_status_4 = (controller_status_4 | 0x40); // left
						if ((usb_buttons[usb_readpos] & 0x0800) == 0x0800) controller_status_4 = (controller_status_4 | 0x80); // right
						if ((usb_buttons[usb_readpos] & 0x2000) == 0x2000) controller_status_4 = (controller_status_4 | 0x04); // select
						if ((usb_buttons[usb_readpos] & 0x1000) == 0x1000) controller_status_4 = (controller_status_4 | 0x08); // start
						//if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) controller_status_4 = (controller_status_4 | 0x01); // X
						//if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) controller_status_4 = (controller_status_4 | 0x02); // Y
						//if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) controller_status_4 = (controller_status_4 | 0x02); // A
						//if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) controller_status_4 = (controller_status_4 | 0x01); // B
						
						if (controller_mapping == 1) // changed
						{
							if ((usb_buttons[usb_readpos] & 0x0040) == 0x0040) controller_status_4 = (controller_status_4 | 0x02); // X
							if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) controller_status_4 = (controller_status_4 | 0x01); // A
						}
						else if (controller_mapping == 2) // changed
						{
							if ((usb_buttons[usb_readpos] & 0x0080) == 0x0080) controller_status_4 = (controller_status_4 | 0x02); // Y
							if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) controller_status_4 = (controller_status_4 | 0x01); // B
						}
						else // default
						{
							if ((usb_buttons[usb_readpos] & 0x0010) == 0x0010) controller_status_4 = (controller_status_4 | 0x02); // A
							if ((usb_buttons[usb_readpos] & 0x0020) == 0x0020) controller_status_4 = (controller_status_4 | 0x01); // B
						}

						usb_readpos++;
					}
				}
			}
			else
			{
				controller_status_4 = 0x00;
			}
			
			screen_speed_dir = 1;
			
			if (controller_mapping == 0) // default Genesis controller
			{
				if (controller_mode > 0)
				{
					controller_status_2 = (controller_status_2 & 0x0C);
					controller_status_3 = (controller_status_3 & 0x0C);

					if (controller_detect_1 > 0)
					{
						controller_status_2 = controller_status_2 | 
							((!PORTKbits.RK0) << 4) | // up
							((!PORTKbits.RK1) << 5) | // down
							((!PORTKbits.RK2) << 6) | // left
							((!PORTKbits.RK3) << 7) | // right
							((!PORTKbits.RK4) << 1) | // B
							((!PORTKbits.RK5)); // A
					}

					if (controller_detect_2 > 0)
					{
						controller_status_3 = controller_status_3 | 
							((!PORTFbits.RF0) << 4) | // up
							((!PORTFbits.RF1) << 5) | // down
							((!PORTFbits.RF2) << 6) | // left
							((!PORTFbits.RF4) << 7) | // right
							((!PORTFbits.RF5) << 1) | // B
							((!PORTFbits.RF8)); // A
					}
				}
				else
				{
					if ((!PORTKbits.RK2) && (!PORTKbits.RK3) && PORTKbits.RK0 && PORTKbits.RK1)
					{
						controller_detect_1 = 0x01;
					}

					if ((!PORTFbits.RF2) && (!PORTFbits.RF4) && PORTFbits.RF0 && PORTFbits.RF1)
					{
						controller_detect_2 = 0x01;
					}

					controller_status_2 = (controller_status_2 & 0xF3);
					controller_status_3 = (controller_status_3 & 0xF3);

					if (controller_detect_1 > 0)
					{
						controller_status_2 = controller_status_2 |
							((!PORTKbits.RK4) << 2) | // select
							((!PORTKbits.RK5) << 3); // start
					}

					if (controller_detect_2 > 0)
					{
						controller_status_3 = controller_status_3 |
							((!PORTFbits.RF5) << 2) | // select
							((!PORTFbits.RF8) << 3); // start
					}
				}
			}
			else // GameTank controller layout
			{
				if (controller_mode > 0)
				{
					controller_status_2 = (controller_status_2 & 0x0C);
					controller_status_3 = (controller_status_3 & 0x0C);

					if (controller_detect_1 > 0)
					{
						controller_status_2 = controller_status_2 | 
							((!PORTKbits.RK0) << 4) | // up
							((!PORTKbits.RK1) << 5) | // down
							((!PORTKbits.RK2) << 6) | // left
							((!PORTKbits.RK3) << 7) | // right
							((!PORTKbits.RK4)) | // A
							((!PORTKbits.RK5) << 2); // select
					}

					if (controller_detect_2 > 0)
					{
						controller_status_3 = controller_status_3 | 
							((!PORTFbits.RF0) << 4) | // up
							((!PORTFbits.RF1) << 5) | // down
							((!PORTFbits.RF2) << 6) | // left
							((!PORTFbits.RF4) << 7) | // right
							((!PORTFbits.RF5)) | // A
							((!PORTFbits.RF8) << 2); // select
					}
				}
				else
				{
					if ((!PORTKbits.RK2) && (!PORTKbits.RK3) && PORTKbits.RK0 && PORTKbits.RK1)
					{
						controller_detect_1 = 0x01;
					}

					if ((!PORTFbits.RF2) && (!PORTFbits.RF4) && PORTFbits.RF0 && PORTFbits.RF1)
					{
						controller_detect_2 = 0x01;
					}

					controller_status_2 = (controller_status_2 & 0xF3);
					controller_status_3 = (controller_status_3 & 0xF3);

					if (controller_detect_1 > 0)
					{
						controller_status_2 = controller_status_2 |
							((!PORTKbits.RK4) << 1) | // B
							((!PORTKbits.RK5) << 3); // start
					}

					if (controller_detect_2 > 0)
					{
						controller_status_3 = controller_status_3 |
							((!PORTFbits.RF5) << 1) | // B
							((!PORTFbits.RF8) << 3); // start
					}
				}
			}
			
			controller_status_1 = 0x00;

			if (controller_mapping == 1) // changed
			{
				controller_status_1 = controller_status_1 | 
					((!PORTBbits.RB8) << 4) | // up
					((!PORTBbits.RB9) << 5) | // down
					((!PORTBbits.RB10) << 6) | // left
					((!PORTBbits.RB11) << 7) | // right
					((!PORTDbits.RD14) << 2) | // select
					((!PORTDbits.RD15) << 3) | // start
					((!PORTBbits.RB15) << 1) | // B
					((!PORTBbits.RB13)); // A
				
				if (PORTBbits.RB14 == 0) screen_speed_dir = 2;
				if (PORTBbits.RB12 == 0) screen_speed_dir = 0;
			}
			else if (controller_mapping == 2) // changed
			{
				controller_status_1 = controller_status_1 | 
					((!PORTBbits.RB8) << 4) | // up
					((!PORTBbits.RB9) << 5) | // down
					((!PORTBbits.RB10) << 6) | // left
					((!PORTBbits.RB11) << 7) | // right
					((!PORTDbits.RD14) << 2) | // select
					((!PORTDbits.RD15) << 3) | // start
					((!PORTBbits.RB14) << 1) | // B
					((!PORTBbits.RB12)); // A
				
				if (PORTBbits.RB15 == 0) screen_speed_dir = 2;
				if (PORTBbits.RB13 == 0) screen_speed_dir = 0;		
			}
			else // default
			{
				controller_status_1 = controller_status_1 | 
					((!PORTBbits.RB8) << 4) | // up
					((!PORTBbits.RB9) << 5) | // down
					((!PORTBbits.RB10) << 6) | // left
					((!PORTBbits.RB11) << 7) | // right
					((!PORTDbits.RD14) << 2) | // select
					((!PORTDbits.RD15) << 3) | // start
					((!PORTBbits.RB13) << 1) | // B
					((!PORTBbits.RB12)); // A
					//((!PORTBbits.RB14) << 1) | // X
					//((!PORTBbits.RB15)); // Y
				
				if (PORTBbits.RB14 == 0) screen_speed_dir = 2;
				if (PORTBbits.RB15 == 0) screen_speed_dir = 0;
			}
		}
		
		if (controller_mode > 0)
		{
			PORTKbits.RK6 = 0; // ground when not floating
			TRISKbits.TRISK6 = 0;
			
			controller_mode = 0;
		}
		else
		{
			PORTKbits.RK6 = 0;
			TRISKbits.TRISK6 = 1; // high when floating
			
			controller_mode = 1;
		}
	}
	else
	{
		PORTKbits.RK6 = 0;
		TRISKbits.TRISK6 = 1; // high when floating
		
		controller_mode = 1;
	}
	
	if (screen_speed_mode == 0) screen_speed_dir = 1; 
}

volatile void __attribute__((vector(_OUTPUT_COMPARE_3_VECTOR), interrupt(ipl7srs))) oc3_handler()
{	
	IFS0bits.OC3IF = 0;  // clear interrupt flag
	
	PORTH = 0;
	
	screen_scanline = screen_scanline + 1; // increment scanline
	
	if (screen_scanline == 806)
	{
		screen_scanline = 0;
		
		screen_sync++; // used to check if screen has finished
		
#ifdef DEBUG
debug_report();
#endif
	}
	
	if (screen_handheld > 0)
	{
		if (lcd_request == lcd_column+1)
		{
			if (lcd_column > 1)
			{
				lcd_ready = 1;
			}
			else
			{
				lcd_half();
			}
		}
	}
	
	if (screen_scanline == 0)
	{
		if (screen_handheld == 0)
		{
			DCH1INTbits.CHBCIF = 0; // clear transfer complete flag
			DCH1SSA = VirtToPhys(screen_line); // transfer source physical address	
			DCH1CONbits.CHEN = 1; // enable channel
		}
	}
	else if (screen_scanline < SCREEN_Y*3)
	{	
		if (screen_handheld == 0)
		{
			DCH1INTbits.CHBCIF = 0; // clear transfer complete flag
			DCH1SSA = VirtToPhys(screen_buffer + SCREEN_XY*screen_frame + SCREEN_X*(unsigned long)(((screen_scanline)/3))); // transfer source physical address
			DCH1CONbits.CHEN = 1; // enable channel
		}
	}
	else if (screen_scanline == SCREEN_Y*3)
	{
		if (screen_handheld == 0)
		{
			DCH1INTbits.CHBCIF = 0; // clear transfer complete flag
			DCH1SSA = VirtToPhys(screen_line); // transfer source physical address
			DCH1CONbits.CHEN = 1; // enable channel
		}
	}
	else if (screen_scanline == SCREEN_Y*3+1)
	{
		update_controllers();
	}
	
	update_usb();
	
	return;
}

volatile void __attribute__((vector(_DMA3_VECTOR), interrupt(ipl6srs))) dma3_handler()
{	
	IFS4bits.DMA3IF = 0;  // clear interrupt flag
	
	if (lcd_request > 0) lcd_request++;
}

void __attribute__((vector(_USB_VECTOR), interrupt(ipl5srs), nomips16)) usb_handler()
{
	unsigned long CSR0 = USBCSR0; // must read to clear?
	unsigned long CSR1 = USBCSR1;
	unsigned long CSR2 = USBCSR2;

	if ((CSR0 & 0x00010000) > 0) // EP0IF bit
	{
		USBCSR0bits.EP0IF = 0; // clear flag
		usbhost_ep0_interrupt = 1; // set flag
	}
	
	if ((CSR1 & 0x00000002) > 0) // EP1RXIF bit
	{
		USBCSR1bits.EP1RXIF = 0; // clear flag
		usbhost_ep1_interrupt++; // increment flag
	}

	if ((CSR2 & 0x00100000) > 0) // CONNIF bit
	{
		usbhost_connected = 1; // set flag
	}

	IFS4bits.USBIF = 0; // clear flag
}

volatile void __attribute__((vector(_UART2_RX_VECTOR), interrupt(ipl4srs))) u2rx_handler() //, nomips16)) u3rx_handler()
{	
	IFS4bits.U2RXIF = 0;  // clear interrupt flag
	
	// check for errors
	if(U2STAbits.FERR == 1)
	{
		return;
	}
		
	// clear overrun error
	if(U2STAbits.OERR == 1)
	{
		U2STAbits.OERR = 0;
		return;
	}
	
	// check if there is a character ready to read
	if(U2STAbits.URXDA == 1)
	{
		unsigned char data = U2RXREG; // get character
		
		if (data == '*') // debug?
		{
			
		}
		else
		{
			U2TXREG = data; // echo character received
		}
	}
	
	return;
}

volatile void __attribute__((vector(_TIMER_8_VECTOR), interrupt(ipl3srs))) t8_handler()
{		
	IFS1bits.T8IF = 0;  // clear interrupt flag
	
	if (audio_enable > 0 && (screen_speed_mode == 0 || screen_speed_dir == 1))
	{
		if (audio_bank == 0)
		{
			// 6-bit signed audio add 0x0080, unsigned add 0x0000
			PORTA = (unsigned short)((unsigned char)(((audio_buffer[(audio_read)%AUDIO_LEN] & 0x00FC) * audio_volume) / 4) + 0xC000); // needs 0xC000 to not reset LCD
			PORTB = (unsigned short)((unsigned char)(((audio_buffer[(audio_read)%AUDIO_LEN] & 0x00FC) * audio_volume) / 4) + 0x0000); // doesn't matter

			audio_read = audio_read + 1;

			if (audio_read >= AUDIO_LEN)
			{
				audio_read = 0;
			}
		}
		else if (audio_bank == 1)
		{
			// 6-bit signed audio add 0x0080, unsigned add 0x0000
			PORTA = (unsigned short)((unsigned char)(((audio_buffer[(audio_read)%AUDIO_EXT] & 0x00FC) * audio_volume) / 4) + 0xC080); // needs 0xC000 to not reset LCD
			PORTB = (unsigned short)((unsigned char)(((audio_buffer[(audio_read)%AUDIO_EXT] & 0x00FC) * audio_volume) / 4) + 0x0080); // doesn't matter

			audio_read = audio_read + 1;

			if (audio_read >= AUDIO_EXT)
			{
				audio_read = 0;
			}
		}
		else if (audio_bank == 2)
		{
			// 6-bit signed audio add 0x0080, unsigned add 0x0000
			PORTA = (unsigned short)((unsigned char)(((audio_buffer2[(audio_read)%AUDIO_EXT] & 0x00FC) * audio_volume) / 4) + 0xC080); // needs 0xC000 to not reset LCD
			PORTB = (unsigned short)((unsigned char)(((audio_buffer2[(audio_read)%AUDIO_EXT] & 0x00FC) * audio_volume) / 4) + 0x0080); // doesn't matter

			audio_read = audio_read + 1;

			if (audio_read >= AUDIO_EXT)
			{
				audio_read = 0;
			}
		}
	}
}
