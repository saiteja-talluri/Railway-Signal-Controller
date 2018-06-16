----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    05:12:16 01/23/2018 
-- Design Name: 
-- Module Name:    encrypter - Behavioral 
-- Project Name: 
-- Target Devices: 
-- Tool versions: 
-- Description: 
--
-- Dependencies: 
--
-- Revision: 
-- Revision 0.01 - File Created
-- Additional Comments: 
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
-- use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
-- library UNISIM;
-- use UNISIM.VComponents.all;

entity encrypter is
    Port ( clock : in  STD_LOGIC;
           K : in  STD_LOGIC_VECTOR (31 downto 0);
           P : in  STD_LOGIC_VECTOR (31 downto 0);
           C : out  STD_LOGIC_VECTOR (31 downto 0);
           reset : in  STD_LOGIC;
			  done: out STD_LOGIC;
           enable : in  STD_LOGIC);
end encrypter;



architecture Behavioral of encrypter is
		
		
		signal i : INTEGER range -1 to 32 := -1;  -- For Iteration over the Key 
		signal Ctemp: STD_LOGIC_VECTOR (31 downto 0) := (others => '0'); -- The Virtual Output 
		signal T: STD_LOGIC_VECTOR (3 downto 0) := (others => '0');  

 begin
  process(clock, reset, enable)
   begin
		-- Reset for a new computation ( new K or P)
		if (reset = '1') then
			C     <= "00000000000000000000000000000000";
			--Ctemp <= "00000000000000000000000000000000";
			i <= -1;
			done <= '0';
			-- To start or pause, change the enable
			-- Runs the below code at positive edge of the clock-cycle and enable = 1
		 elsif (clock'event and clock = '1' and enable = '1') then
					if ( i = -1 ) then 
						C <= P;
						Ctemp <= P;
						T(3) <= K(31) xor K(27) xor K(23) xor K(19) xor K(15) xor K(11) xor K(7) xor K(3);
						T(2) <= K(30) xor K(26) xor K(22) xor K(18) xor K(14) xor K(10) xor K(6) xor K(2);
						T(1) <= K(29) xor K(25) xor K(21) xor K(17) xor K(13) xor K(9) xor K(5) xor K(1);
						T(0) <= K(28) xor K(24) xor K(20) xor K(16) xor K(12) xor K(8) xor K(4) xor K(0);
						i <= 0;
						done <= '0';
					
					elsif(i > -1 and i < 32) then  
							if(K(i) = '1') then
								Ctemp <= Ctemp XOR (T&T&T&T&T&T&T&T);	
								T <= T+"0001";
								i <= i+1;
							else 
								i <= i+1;
							end if;
						else 
							C <= Ctemp;
							done <= '1';
					end if;
		end if;	
	-- C is the actual Encrypted output		
	--C <= Ctemp;
  end process;

end Behavioral;

