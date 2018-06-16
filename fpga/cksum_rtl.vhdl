--
-- Copyright (C) 2009-2012 Chris McClelland
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
--
library ieee;

use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use IEEE.NUMERIC_STD.ALL;


architecture rtl of swled is

	component encrypter is
    Port ( clock : in  STD_LOGIC;
        	K : in  STD_LOGIC_VECTOR (31 downto 0);
        	P : in  STD_LOGIC_VECTOR (31 downto 0);
        	C : out  STD_LOGIC_VECTOR (31 downto 0);
        	reset : in  STD_LOGIC;
			done: out STD_LOGIC;
        	enable : in  STD_LOGIC);
	end component;

	component decrypter is
    Port ( clock : in  STD_LOGIC;
			K : in  STD_LOGIC_VECTOR (31 downto 0);
			C : in  STD_LOGIC_VECTOR (31 downto 0);
            P : out  STD_LOGIC_VECTOR (31 downto 0);
            reset : in  STD_LOGIC;
			done: out STD_LOGIC;
        	enable : in  STD_LOGIC);
	end component;

	component basic_uart is
		generic (
			DIVISOR: natural
		);
		port (
			clk: in std_logic;   -- system clock
			reset: in std_logic;
		
			-- Client interface
			rx_data: out std_logic_vector(7 downto 0);  -- received byte
			rx_enable: out std_logic;  -- validates received byte (1 system clock spike)
			tx_data: in std_logic_vector(7 downto 0);  -- byte to send
			tx_enable: in std_logic;  -- validates byte to send if tx_ready is '1'
			tx_ready: out std_logic;  -- if '1', we can send a new byte, otherwise we won't take it

			-- Physical interface
			rx: in std_logic;
			tx: out std_logic
		);
		end component;
		

	type StateType is (
		S_RECEIVE,
		S_SEND,
		S_START,
		S_RESET
	);

	-- Flags for display on the 7-seg decimal points
	constant Coordinate						: STD_LOGIC_VECTOR(31 downto 0)	:= "00100010000000000000000000000000";
	constant ACK1							: STD_LOGIC_VECTOR(31 downto 0)	:= "11111111000000001111111100000000";
	constant ACK2							: STD_LOGIC_VECTOR(31 downto 0)	:= "00000000111111110000000011111111";
	constant Key							: STD_LOGIC_VECTOR(31 downto 0) := "11010100101010111001001011101010";
	constant wait_MAX2						: STD_LOGIC_VECTOR(33 downto 0) := "0000010001001010101000100000000000";
	constant wait_MAX						: STD_LOGIC_VECTOR(33 downto 0) := "1011011100011011000000000000000000";

	signal reset						: STD_LOGIC;
	signal isWait,isWait_next			: STD_LOGIC;
	signal stoWait,stoWait_next			: STD_LOGIC;

	signal reWait,reWait_next				: STD_LOGIC;
	signal rewait_count,rewait_count_next	: INTEGER RANGE 0 TO 144000003	; 

	signal iwait_count,iwait_count_next	: INTEGER RANGE 0 TO 48000002	; 
	signal swait_count,swait_count_next	: INTEGER RANGE 0 TO 9			; 

	signal ongoing, ongoing_next		: STD_LOGIC						;
	signal state, state_next			: StateType						;
	signal isPoint, isPoint_next		: STD_LOGIC						;
	signal isPointCHK, isPointCHK_next	: STD_LOGIC						;
	signal isPointACK, isPointACK_next	: STD_LOGIC						;
	signal isFirInput, isFirInput_next	: STD_LOGIC						;
	signal isSecInput, isSecInput_next	: STD_LOGIC						;
	signal isS, isS_next				: STD_LOGIC						;
	signal isS3, isS3_next				: STD_LOGIC						;
	signal isS4, isS4_next				: STD_LOGIC						;
	signal isS5, isS5_next				: STD_LOGIC						;
	signal isS6, isS6_next				: STD_LOGIC						;

	
	signal toSend, toSend_next			: STD_LOGIC						;
	signal rcount, rcount_next			: INTEGER RANGE 0 TO 9			;			--for the count of inputs
	signal icount, icount_next			: INTEGER RANGE 0 TO 9			;			--for the count of inputs

	signal flags						: std_logic_vector(3 downto 0);
	
	
	signal input1, input1_next			: STD_LOGIC_VECTOR(7 downto 0)	;		-- Registers implementing the channels
	signal input2, input2_next			: STD_LOGIC_VECTOR(7 downto 0)	;
	signal input3, input3_next			: STD_LOGIC_VECTOR(7 downto 0)	;
	signal input4, input4_next			: STD_LOGIC_VECTOR(7 downto 0)	;
	signal input5, input5_next			: STD_LOGIC_VECTOR(7 downto 0)	;
	signal input6, input6_next			: STD_LOGIC_VECTOR(7 downto 0)	;
	signal input7, input7_next			: STD_LOGIC_VECTOR(7 downto 0)	;
	signal input8, input8_next			: STD_LOGIC_VECTOR(7 downto 0)	;

	signal swinput, swinput_next		: STD_LOGIC_VECTOR(7 downto 0)	;

	signal uart_data,uart_data_next			: STD_LOGIC_VECTOR(7 downto 0)	;
	signal uart_txreset,uart_txreset_next 	: STD_LOGIC						;
	signal uart_rxreset,uart_rxreset_next 	: STD_LOGIC						;

	
	signal EnOut_out					: STD_LOGIC_VECTOR(31 downto 0)	;
	signal EnInput_in,EnInput_in_P		: STD_LOGIC_VECTOR(31 downto 0) := Coordinate;		--For running Encrypter Input wire doesn't need to hold the value for the process.
	signal EnDone_out					: STD_LOGIC						;
	signal EnEnable_in, EnEnable_in_next: STD_LOGIC 					;
	signal EnReset_in, EnReset_in_next	: STD_LOGIC 					;

	signal DeOut_out					: STD_LOGIC_VECTOR(31 downto 0)	;
	signal DeInput_in,DeInput_in_P		: STD_LOGIC_VECTOR(31 downto 0)	;		--For running Encrypter Input wire doesn't need to hold the value for the process.
	signal DeDone_out					: STD_LOGIC						;
	signal DeEnable_in, DeEnable_in_next: STD_LOGIC 					;
	signal DeReset_in, DeReset_in_next	: STD_LOGIC 					;

	signal clk_count : integer range 0 to 1540000002 ;

	type fsm_state_t is (idle, received, emitting);
	type state_t is
	record
	fsm_state: fsm_state_t; -- FSM state
	tx_data: std_logic_vector(7 downto 0);
	rx_data: std_logic_vector(7 downto 0);
	tx_enable: std_logic;
	
	tx_done: std_logic;
	rx_done: std_logic;
	
	end record;

	signal uart_state,uart_state_next: state_t;
--	signal led_wait : integer range 0 to 10 := 0;

	signal uart_rx_data					: std_logic_vector(7 downto 0);
	signal uart_rx_enable				: std_logic;
	signal uart_tx_data					: std_logic_vector(7 downto 0);
	signal uart_tx_enable				: std_logic;
	signal uart_tx_ready				: std_logic;
	signal toSend_uart,toSend_uart_next	: std_logic;
begin                                                                     --BEGIN_SNIPPET(registers)
	-- Infer registers
	En: encrypter
	PORT MAP ( 	clock 	=> clk_in,
        		K 		=> Key,
        		P 		=> EnInput_in,
        		C 		=> EnOut_out,
        		reset 	=> EnReset_in,
				done 	=> EnDone_out,
        		enable 	=> EnEnable_in);

	De: decrypter 
	PORT MAP (	clock 	=> clk_in,
				K		=> Key,
				C		=> DeInput_in,
				P		=> DeOut_out,
				reset	=> DeReset_in,
				done	=> DeDone_out,
				enable	=> DeEnable_in);

	-- i_brg : baudrate_gen port map (clk => clk_in, rst => reset_in, sample => sample);
	
	-- i_rx : uart_rx port map( clk => clk_in, rst => uart_rxreset,
    --                         rx => uart_rx_in, sample => sample,
    --                         rxdone => rxdone, rxdata => rxdata);
									
	-- i_tx : uart_tx port map( clk => clk_in, rst => uart_txreset,
    --                         txstart => txstart,
    --                         sample => sample, txdata => txdata,
    --                         txdone => txdone, tx => uart_tx_out);
	basic_uart_inst: basic_uart
	generic map (DIVISOR => 26) -- 2400
	PORT MAP (
				clk => clk_in, 
				reset => reset_in,
				rx_data => uart_rx_data, rx_enable => uart_rx_enable,
				tx_data => uart_tx_data, tx_enable => uart_tx_enable, tx_ready => uart_tx_ready,
				rx => uart_rx_in,
				tx => uart_tx_out);
	
				
	process(clk_in,reset_in)
	begin
		if ( reset_in = '1' ) then
			state			<= S_RESET;
			ongoing			<= '0';
			clk_count		<= 0;
			uart_data		<= "11000000";
			uart_state.fsm_state <= idle;
			uart_state.tx_data <= (others => '0');
			uart_state.tx_enable <= '0';
			uart_state.tx_done <= '0';
			uart_state.rx_done <= '0';
			uart_state.rx_data <= (others => '0');
			
			

			elsif ( rising_edge(clk_in) ) then
			
				reWait			<= reWait_next;
				rewait_count	<= rewait_count_next;
				uart_state		<= uart_state_next;

				state			<= state_next;
				isS				<= isS_next;
				uart_data		 <= uart_data_next;
				toSend_uart		 <= toSend_uart_next;

				if(isWait = '1') then 
								
					if(iwait_count = 48000000) then 
						swait_count <= swait_count +1;
						iwait_count <= 0;
					else 
					iwait_count <= iwait_count + 1;
					--wait_count <= wait_count + "0000000000000000000000000000000001";
					end if;
				else 
					iwait_count			<= iwait_count_next;
					swait_count			<= swait_count_next;
					--wait_count		<= wait_count_next;
				end if;
				

				if(reWait = '1')	then 
					if(rewait_count = 144000000) then 
						led_out	<= "00000000";
						rewait_count	<= 0;
						reWait			<= '0';
					else
						if(rewait_count = 1) then 
						led_out			<= "11111111";
						end if;
						rewait_count	<= rewait_count + 1;
						
					end if;
				
				end if;
				
				if (ongoing = '1') then 
						
					if(clk_count = 0) then
						if(uart_data(5 downto 3)="000")then
							if(input1(7)='1') then
								if(input1(6)='1' and uart_data(6)='1') then
									if(input1(2 downto 0) < uart_data(2 downto 0)) then
										input1(2 downto 0) <= uart_data(2 downto 0);
									end if;
								else 
									input1(6)<='0';
								end if;
							end if;
						elsif(uart_data(5 downto 3)="001")then
							if(input2(7)='1') then
								if(input2(6)='1' and uart_data(6)='1') then
									if(input2(2 downto 0) < uart_data(2 downto 0)) then
										input2(2 downto 0) <= uart_data(2 downto 0);
									end if;
								else 
									input2(6)<='0';
								end if;
							end if;
						elsif(uart_data(5 downto 3)="010")then
							if(input3(7)='1') then
								if(input3(6)='1' and uart_data(6)='1') then
									if(input3(2 downto 0) < uart_data(2 downto 0)) then
										input3(2 downto 0) <= uart_data(2 downto 0);
									end if;
								else 
									input3(6)<='0';
								end if;
							end if;
						elsif(uart_data(5 downto 3)="011")then
							if(input4(7)='1') then
								if(input4(6)='1' and uart_data(6)='1') then
									if(input4(2 downto 0) < uart_data(2 downto 0)) then
										input4(2 downto 0) <= uart_data(2 downto 0);
									end if;
								else 
									input4(6)<='0';
								end if;
							end if;
						elsif(uart_data(5 downto 3)="100")then
							if(input5(7)='1') then
								if(input5(6)='1' and uart_data(6)='1') then
									if(input5(2 downto 0) < uart_data(2 downto 0)) then
										input5(2 downto 0) <= uart_data(2 downto 0);
									end if;
								else 
									input5(6)<='0';
								end if;
							end if;
						elsif(uart_data(5 downto 3)="101")then
							if(input6(7)='1') then
								if(input6(6)='1' and uart_data(6)='1') then
									if(input6(2 downto 0) < uart_data(2 downto 0)) then
										input6(2 downto 0) <= uart_data(2 downto 0);
									end if;
								else 
									input6(6)<='0';
								end if;
							end if;
						elsif(uart_data(5 downto 3)="110")then
							if(input7(7)='1') then
								if(input7(6)='1' and uart_data(6)='1') then
									if(input7(2 downto 0) < uart_data(2 downto 0)) then
										input7(2 downto 0) <= uart_data(2 downto 0);
									end if;
								else 
									input7(6)<='0';
								end if;
							end if;
						elsif(uart_data(5 downto 3)="111")then
							if(input8(7)='1') then
								if(input8(6)='1' and uart_data(6)='1') then
									if(input8(2 downto 0) < uart_data(2 downto 0)) then
										input8(2 downto 0) <= uart_data(2 downto 0);
									end if;
								else 
									input8(6)<='0';
								end if;
							end if;
						end if;
					end if;
					if(clk_count = 1) then 
						if( input1(7) = '0' OR input1(7 downto 6) = "10") then 
							led_out <=	"10000"&input1(5 downto 3);
						
						else
							if(swinput(0) = '1') then
								if(swinput(4)='1') then
									led_out <=	"10000"&input1(5 downto 3);
								else
									if(unsigned(input1(2 downto 0))=1) then
										led_out <=	"01000"&input1(5 downto 3);
									else
										led_out <=	"00100"&input1(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input1(5 downto 3);
							end if;
						end if;
					end if;
							
					if(clk_count = 144000000) then 
						if( input2(7) = '0' OR input2(7 downto 6) = "10") then 
							led_out <=	"10000"&input2(5 downto 3);
						
						else
							if(swinput(1 ) = '1') then
								if(swinput(5)='1') then
									led_out <=	"10000"&input2(5 downto 3);
								else
									if(unsigned(input2(2 downto 0))=1) then
										led_out <=	"01000"&input2(5 downto 3);
									else
										led_out <=	"00100"&input2(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input2(5 downto 3);end if;
						end if;
					end if;

					if(clk_count = 288000000) then 
						if( input3(7) = '0' OR input3(7 downto 6) = "10") then 
							led_out <=	"10000"&input3(5 downto 3);
						
						else
							if(swinput(2) = '1') then
								if(swinput(6)='1') then
									led_out <=	"10000"&input3(5 downto 3);
								else
									if(unsigned(input3(2 downto 0))=1) then
										led_out <=	"01000"&input3(5 downto 3);
									else
										led_out <=	"00100"&input3(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input3(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 432000000) then 
						if( input4(7) = '0' OR input4(7 downto 6) = "10") then 
							led_out <=	"10000"&input4(5 downto 3);
						
						else
							if(swinput(3) = '1') then
								if(swinput(7)='1') then
									led_out <=	"10000"&input4(5 downto 3);
								else
									if(unsigned(input4(2 downto 0))=1) then
										led_out <=	"01000"&input4(5 downto 3);
									else
										led_out <=	"00100"&input4(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input4(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 576000000) then 
						if( input5(7) = '0' OR input5(7 downto 6) = "10") then 
							led_out <=	"10000"&input5(5 downto 3);
						
						else
							if(swinput(4) = '1') then
								if(swinput(0)= '1') then
									led_out <=	"00100"&input5(5 downto 3);
								else
									if(unsigned(input5(2 downto 0))=1) then
										led_out <=	"01000"&input5(5 downto 3);
									else
										led_out <=	"00100"&input5(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input5(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 624000000) then 
						if( input5(7) = '0' OR input5(7 downto 6) = "10") then 
							led_out <=	"10000"&input5(5 downto 3);
						
						else
							if(swinput(4) = '1') then
								if(swinput(0)= '1') then
									led_out <=	"01000"&input5(5 downto 3);
								else
									if(unsigned(input5(2 downto 0))=1) then
										led_out <=	"01000"&input5(5 downto 3);
									else
										led_out <=	"00100"&input5(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input5(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 672000000) then 
						if( input5(7) = '0' OR input5(7 downto 6) = "10") then 
							led_out <=	"10000"&input5(5 downto 3);
						
						else
							if(swinput(4) = '1') then
								if(swinput(0)= '1') then
									led_out <=	"10000"&input5(5 downto 3);
								else
									if(unsigned(input5(2 downto 0))=1) then
										led_out <=	"01000"&input5(5 downto 3);
									else
										led_out <=	"00100"&input5(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input5(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 720000000) then 
						if( input6(7) = '0' OR input6(7 downto 6) = "10") then 
							led_out <=	"10000"&input6(5 downto 3);
						
						else
							if(swinput(5) = '1') then
								if(swinput(1)= '1') then
									led_out <=	"00100"&input6(5 downto 3);
								else
									if(unsigned(input6(2 downto 0))=1) then
										led_out <=	"01000"&input6(5 downto 3);
									else
										led_out <=	"00100"&input6(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input6(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 768000000) then 
						if( input6(7) = '0' OR input6(7 downto 6) = "10") then 
							led_out <=	"10000"&input6(5 downto 3);
						
						else
							if(swinput(5) = '1') then
								if(swinput(1)= '1') then
									led_out <=	"01000"&input6(5 downto 3);
								else
									if(unsigned(input6(2 downto 0))=1) then
										led_out <=	"01000"&input6(5 downto 3);
									else
										led_out <=	"00100"&input6(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input6(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 816000000) then 
						if( input6(7) = '0' OR input6(7 downto 6) = "10") then 
							led_out <=	"10000"&input6(5 downto 3);
						
						else
							if(swinput(5) = '1') then
								if(swinput(1)= '1') then
									led_out <=	"10000"&input6(5 downto 3);
								else
									if(unsigned(input6(2 downto 0))=1) then
										led_out <=	"01000"&input6(5 downto 3);
									else
										led_out <=	"00100"&input6(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input6(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 864000000) then 
						if( input7(7) = '0' OR input7(7 downto 6) = "10") then 
							led_out <=	"10000"&input7(5 downto 3);
						
						else
							if(swinput(6) = '1') then
								if(swinput(2)= '1') then
									led_out <=	"00100"&input7(5 downto 3);
								else
									if(unsigned(input7(2 downto 0))=1) then
										led_out <=	"01000"&input7(5 downto 3);
									else
										led_out <=	"00100"&input7(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input7(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 912000000) then 
						if( input7(7) = '0' OR input7(7 downto 6) = "10") then 
							led_out <=	"10000"&input7(5 downto 3);
						
						else
							if(swinput(6) = '1') then
								if(swinput(2)= '1') then
									led_out <=	"01000"&input7(5 downto 3);
								else
									if(unsigned(input7(2 downto 0))=1) then
										led_out <=	"01000"&input7(5 downto 3);
									else
										led_out <=	"00100"&input7(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input7(5 downto 3);end if;
						end if;
					end if;
					if(clk_count = 960000000) then 
						if( input7(7) = '0' OR input7(7 downto 6) = "10") then 
							led_out <=	"10000"&input7(5 downto 3);
						
						else
							if(swinput(6) = '1') then
								if(swinput(2)= '1') then
									led_out <=	"10000"&input7(5 downto 3);
								else
									if(unsigned(input7(2 downto 0))=1) then
										led_out <=	"01000"&input7(5 downto 3);
									else
										led_out <=	"00100"&input7(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input7(5 downto 3);
								end if;
								end if;
					end if;
					if(clk_count = 1008000000) then 
						if( input8(7) = '0' OR input8(7 downto 6) = "10") then 
							led_out <=	"10000"&input8(5 downto 3);
						
						else
							if(swinput(7) = '1') then
								if(swinput(3)= '1') then
									led_out <=	"00100"&input8(5 downto 3);
								else
									if(unsigned(input8(2 downto 0))=1) then
										led_out <=	"01000"&input8(5 downto 3);
									else
										led_out <=	"00100"&input8(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input8(5 downto 3);
								end if;
							end if;
					end if;
					if(clk_count = 1056000000) then 
						if( input8(7) = '0' OR input8(7 downto 6) = "10") then 
							led_out <=	"10000"&input8(5 downto 3);
						
						else
							if(swinput(7) = '1') then
								if(swinput(3)= '1') then
									led_out <=	"01000"&input8(5 downto 3);
								else
									if(unsigned(input8(2 downto 0))=1) then
										led_out <=	"01000"&input8(5 downto 3);
									else
										led_out <=	"00100"&input8(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input8(5 downto 3);
							end if;	
						end if;
					end if;
					if(clk_count = 1104000000) then 
						if( input8(7) = '0' OR input8(7 downto 6) = "10") then 
							led_out <=	"10000"&input8(5 downto 3);
						
						else
							if(swinput(7) = '1') then
								if(swinput(3)='1') then
									led_out <=	"10000"&input8(5 downto 3);
								else
									if(unsigned(input8(2 downto 0))=1) then
										led_out <=	"01000"&input8(5 downto 3);
									else
										led_out <=	"00100"&input8(5 downto 3);
									end if;
								end if;
							else
								led_out <= "10000" & input8(5 downto 3);
							end if;
						end if;
					end if;

						if (clk_count = 1152000000) then 
							led_out <= uart_data;
						end if;
						
						if(clk_count = 1296000000) then
							ongoing		 		<= '0';
							clk_count			<= 	0 ;
							led_out 			<= "00000000";
							uart_data			<= "11000000";
							--swait_count			<=  0 ;
							--iwait_count			<=  0 ;

							if(up = '1') then 
								state				<= S_SEND;
								isS					<= '1';
								isS3				<= '1';
								toSend				<=	'1';
								EnReset_in 	<= '0';
								EnEnable_in	<= '1';
								
							elsif (left = '1') then 
								state				<= S_SEND;
								isS					<= '1';
								isS4				<= '1';
								else
									state				<= S_SEND;
									isS					<= '1';
									isS5				<= '1';
							end if;
						else						
						clk_count <= clk_count +1;
						end if;

						-- if(clk_count = 1536000000) then
						-- 	ongoing		 		<= '0';
						-- 	clk_count			<= 	0;
						-- 	if(up = '1') then 
						-- 		state				<= S_SEND;
						-- 		isS					<= '1';
						-- 		isS3				<= '1';
						-- 	elsif (left = '1') then 
						-- 		state				<= S_SEND;
						-- 		isS					<= '1';
						-- 		isS4				<= '1';
						-- 		else 
						-- 			state				<= S_START;
						-- 	end if;
						-- else						
						-- clk_count <= clk_count +1;
						-- end if;
					
				else 

					
					isS3			<= isS3_next;
					isS4			<= isS4_next;
					isS5			<= isS5_next;
					isS6			<= isS6_next;

					rcount 			<= rcount_next;
					icount 			<= icount_next;
					
					EnInput_in_P	<= EnInput_in;
					DeInput_in_P	<= DeInput_in;
					
					ongoing 		<= ongoing_next;
					swinput			<= swinput_next;

					isWait			<= isWait_next;
					stoWait			<= stoWait_next;

					toSend			<= toSend_next;
					isPoint			<= isPoint_next;
					isPointCHK		<= isPointCHK_next;
					isPointACK		<= isPointACK_next;

					isFirInput		<= isFirInput_next;
					isSecInput		<= isSecInput_next;
					
					EnEnable_in 	<= EnEnable_in_next;
					EnReset_in 		<= EnReset_in_next;
					DeEnable_in 	<= DeEnable_in_next;
					DeReset_in	 	<= DeReset_in_next;

					uart_txreset	<= uart_txreset_next;
					uart_rxreset	<= uart_rxreset_next;
					
					input1			 <= input1_next;
					input2			 <= input2_next;
					input3			 <= input3_next;
					input4			 <= input4_next;
					input5			 <= input5_next;
					input6			 <= input6_next;
					input7			 <= input7_next;
					input8			 <= input8_next;
					
				end if;
			end if;
		
	end process;

	process(state,f2hReady_in,h2fValid_in,rcount,icount,DeDone_out,EnDone_out,isPointACK,toSend,swait_count,isS,isS3,isS4,isS5,uart_state,toSend_uart)
		begin
			state_next 		<= state;
			rcount_next 	<= rcount;
			icount_next		<= icount;
			toSend_next		<= toSend;
			isPoint_next	<= isPoint;
			isPointCHK_next	<= isPointCHK;
			isPointACK_next	<= isPointACK;
			ongoing_next	<= ongoing;
			swinput_next	<= swinput;


			isWait_next			<= isWait;
			stoWait_next		<= stoWait;

			reWait_next			<= reWait;
			rewait_count_next	<= rewait_count;
			--wait_count_next		<= wait_count;
			iwait_count_next	<= iwait_count;
			swait_count_next	<= swait_count;

			EnEnable_in_next	<= EnEnable_in;
			DeEnable_in_next	<= DeEnable_in;
			EnReset_in_next		<= EnReset_in;
			DeReset_in_next		<= DeReset_in;

			isFirInput_next		<= isFirInput;
			isSecInput_next		<= isSecInput;

			f2hData_out 		<= "00000000";

			DeInput_in			<= DeInput_in_P;
			EnInput_in			<= EnInput_in_P;

			input1_next <= input1;
			input2_next <= input2;
			input3_next <= input3;
			input4_next <= input4;
			input5_next <= input5;
			input6_next <= input6;
			input7_next <= input7;
			input8_next <= input8;

			uart_data_next	<= uart_data;

			uart_txreset_next	<= uart_txreset;
			uart_rxreset_next	<= uart_rxreset;

			toSend_uart_next	<= toSend_uart;

			isS_next	<= isS;
			isS3_next	<= isS3;
			isS4_next	<= isS4;
			isS5_next	<= isS5;
			isS6_next	<= isS6;

			

			--f2hValid_out 		<= '0';
			



			CASE state IS
			when S_RESET =>
					state_next			<= S_START;
					rewait_count_next	<=	 0 ;
					reWait_next			<=  '1';
					-- EnInput_in			<= Coordinate;
					

			when S_START =>
					if(reWait = '0') then 
						state_next				<= S_SEND;

						isS_next				<= '0';
						isS3_next				<= '0';
						isS4_next				<= '0';
						isS5_next				<= '0';
						isS6_next				<= '0';

						swinput_next		<= (others => '0');

						isWait_next	<= '0';
						reWait_next	<= '0';
						rewait_count_next	<= 0;

						stoWait_next	<= '0';
						iwait_count_next	<= 0; 
						swait_count_next	<= 0; 

						
						ongoing_next	<= '0';
						isPoint_next	<= '1';
						isPointCHK_next	<= '0';
						isPointACK_next	<= '0';
						isFirInput_next	<= '1';
						isSecInput_next	<= '0';
						
						toSend_next		<= '0';
						rcount_next		<= 1;			--for the count of inputs
						icount_next		<= 1;			--for the count of inputs

						
						
						input1_next			<= (others => '0');		-- Registers implementing the channels
						input2_next			<= (others => '0');
						input3_next			<= (others => '0');
						input4_next			<= (others => '0');
						input5_next			<= (others => '0');
						input6_next			<= (others => '0');
						input7_next			<= (others => '0');
						input8_next			<= (others => '0');

						uart_txreset_next		<= '1';
						uart_rxreset_next		<= '1';
						
						
						EnInput_in			<= Coordinate;		--For running Encrypter Input wire doesn't need to hold the value for the process.
						EnEnable_in_next 	<= '1';
						EnReset_in_next		<= '0';

						
						DeInput_in		<= (others => '0');		--For running Encrypter Input wire doesn't need to hold the value for the process.
						DeEnable_in_next<= '0';
						DeReset_in_next	<= '1';
						toSend_uart_next		<= '0';
						--f2hValid_out 			<= '0';
						-- txstart			<= '0';
						-- txdata			<= (others => '0');

						

					end if;
				when S_SEND =>
					--Assuming Encrptor input has the coordinates. --EnDome = 1
						
						IF(toSend = '1') THEN 
		
							IF(chanAddr_in = "0000110" and f2hReady_in = '1' AND EnDone_out = '1') then 
							--f2hValid_out 	<= '1';
								IF   ( rcount = 1 ) THEN f2hData_out <= EnOut_out(31 DOWNTO 24); rcount_next <= rcount + 1; 
								ELSIF( rcount = 2 ) THEN f2hData_out <= EnOut_out(23 DOWNTO 16); rcount_next <= rcount + 1;
								ELSIF( rcount = 3 ) THEN f2hData_out <= EnOut_out(15 DOWNTO  8); rcount_next <= rcount + 1;
								ELSIF( rcount = 4 ) THEN f2hData_out <= EnOut_out(7  DOWNTO  0); rcount_next <= rcount + 1;
								END IF;
							ELSIF( rcount = 5 ) THEN

								if(stoWait = '1') then 
									isWait_next		<= '1';
									stoWait_next	<= '0';
									swait_count_next		<=  0 ;
									iwait_count_next		<=  0 ;
								end if;
								if(isS	= '0' )then 
									state_next			<= S_RECEIVE;
								end if;
								EnEnable_in_next	<= '0'; 
								EnReset_in_next		<= '1';
								toSend_next			<= '0';
								rcount_next			<=  1 ;	
								--f2hValid_out 		<= '0';
								
							END IF;	
							
						else 
								if(isPoint = '1') then
									if(isPointCHK = '1' AND DeDone_out = '1') then 
										if(DeOut_out = Coordinate) then 
											stoWait_next			<= '1';						--After sending ACK1 (after checking coordinate) start the wait 
											DeReset_in_next 		<= '1';
											DeEnable_in_next		<= '0';
											toSend_next 			<= '1';
											isWait_next				<= '0';
											
											--wait_count_next			<= (others => '0');
										else 
											state_next				<= S_START;
										end if;
									elsif(isPointCHK = '0') then 
											toSend_next 	<= '1';
											stoWait_next	<= '1';							--After sending coordinate start the wait 
											
									end if;
								else
									if (ongoing = '1') then 

									else
										if(isS = '0') then 
												if(isPointACK = '0' AND DeDone_out = '1') then 
													if(DeOut_out = ACK2) then
														state_next			<= S_RECEIVE;
														isPointACK_next 		<= '1';
														DeReset_in_next 		<= '1';
														DeEnable_in_next		<= '0';
														isWait_next				<= '0';
														--wait_count_next			<= (others => '0');
													else 
														state_next				<= S_START;
													end if;
												elsif(isPointACK = '1' AND isFirInput = '1' AND DeDone_out = '1') then 
														input1_next <= DeOut_out(31 downto 24);
														input2_next <= DeOut_out(23 downto 16);
														input3_next <= DeOut_out(15 downto  8);
														input4_next <= DeOut_out(7  downto  0);
														DeReset_in_next 		<= '1';
														DeEnable_in_next	 	<= '0';
														toSend_next				<= '1';
														isFirInput_next 		<= '0';
														isSecInput_next 		<= '1';
												elsif(isSecInput = '1'AND DeDone_out = '1') then 
															input5_next <= DeOut_out(31 downto 24);
															input6_next <= DeOut_out(23 downto 16);
															input7_next <= DeOut_out(15 downto  8);
															input8_next <= DeOut_out(7  downto  0);
															DeReset_in_next 		<= '1';
															DeEnable_in_next	 	<= '0';
															toSend_next				<= '1';
															isSecInput_next			<= '0';
															stoWait_next			<= '1';				--After sending ACK1 (after receiving the last set of input) start the wait 
												elsif(isSecInput = '0' AND isFirInput = '0' AND DeDone_out = '1') then
																if(DeOut_out = ACK2) then 
																	ongoing_next		<= '1';
																	swinput_next		<= sw_in;
																	DeReset_in_next 	<= '1';
																	DeEnable_in_next	<= '0';
																	toSend_next			<= '0';
																	isWait_next			<= '0';

																	swait_count_next		<=  0 ;
																	iwait_count_next		<=  0 ;

																	EnEnable_in_next	<= '0'; 
																	EnReset_in_next		<= '1';
																	EnInput_in			<="11111111000000000000000000000000";
																	--wait_count_next		<= (others => '0');
																else 
																	state_next				<= S_START;
																end if;
												end if;
										else 
											if(isS3 = '1') then 
												
												if(down = '1') then
													EnReset_in_next 	<= '0';
													EnInput_in			<= sw_in&"000000000000000000000000";
													EnEnable_in_next	<= '1';
													toSend_next			<= '1';
													isS3_next			<= '0';
													if(left = '1') then 
														isS4_next			<= '1';
													else 
														isS5_next			<= '1';
													end if;
												end if;
											end if;
											if(isS4 = '1') then 
													if(right = '1' ) then 
														-- txdata 			<= sw_in;
														-- txstart			<= '1';
														isS4_next		<= '0';
														isS5_next		<= '1';
														--uart_txreset_next	<= '0';
														toSend_uart_next	<= '1';
													end if;
											end if;
											if(isS5 = '1') then 
													-- if (rxdone	= '1' and uart_rxreset <= '0') then 
													-- 	uart_data_next 			<= rxdata;
													-- 	state_next				<= S_RESET;
													-- 	end if;
														if(uart_state.rx_done = '1')then 
															uart_data_next			<= uart_state.rx_data;
															
														end if;
														isS5_next				<= '0';
														
														isS6_next				<= '1';
														toSend_uart_next	<= '0';
													
														
														--isWait_next				<= '1';
													
											end if;
											if(isS6 = '1')then 
												--if(iwait_count > 48000000) then 
													state_next				<= S_RECEIVE;
													isWait_next				<= '1';
													swait_count_next		<=  0 ;
													iwait_count_next		<=  0 ;
													isS6_next				<= '0';
												--	isWait_next				<= '0';
												--end if;
											end if;

										end if;
									end if;
									
								end if;
						end if;
									
				
				when S_RECEIVE =>
					if(isS = '1' ) then
						if(swait_count > 5) then 
							state_next				<= S_RESET;
							isWait_next				<= '0';
							swait_count_next		<=  0 ;
							iwait_count_next		<=  0 ;
						end if;
					elsif(swait_count < 8) then
						IF(chanAddr_in = "0000111" and h2fValid_in = '1') THEN
							icount_next <= icount + 1;
							IF    ( icount = 1 ) THEN DeInput_in(31 DOWNTO 24) <= h2fData_in;
							ELSIF ( icount = 2 ) THEN DeInput_in(23 DOWNTO 16) <= h2fData_in;
							ELSIF ( icount = 3 ) THEN DeInput_in(15 DOWNTO  8) <= h2fData_in;
							ELSIF ( icount = 4 ) THEN DeInput_in(7  DOWNTO  0) <= h2fData_in;
							END IF;
						ELSIF( icount = 5 ) THEN 
												
												icount_next	<=  1 ;
												state_next	<= S_SEND;

												if(isPoint = '1') then 
													if(isPointCHK = '1' ) then
														DeReset_in_next 		<= '0';
														DeEnable_in_next	 	<= '1';
														isPoint_next			<= '0';
													else 
														EnReset_in_next 			<= '0';
														DeReset_in_next				<= '0';
														EnInput_in					<= ACK1;
														EnEnable_in_next			<= '1';
														DeEnable_in_next		 	<= '1';
														isPointCHK_next				<= '1';
													end if;
												else 
													if (ongoing = '1') then 
											
													else 
												
														if(isFirInput = '1') then 
															
															EnReset_in_next 	<= '0';
															EnInput_in			<= ACK1;
															EnEnable_in_next	<= '1';
															DeReset_in_next		<= '0';
															DeEnable_in_next 	<= '1';
														elsif(isSecInput = '1') then 
																
															EnReset_in_next 	<= '0';
															EnInput_in			<= ACK1;
															EnEnable_in_next	<= '1';
															DeReset_in_next		<= '0';
															DeEnable_in_next 	<= '1';
														elsif(isSecInput = '0' AND isFirInput = '0') then 
																	
															EnReset_in_next 	<= '0';
															EnInput_in			<= Coordinate;
															EnEnable_in_next	<= '1';
															DeReset_in_next		<= '0';
															DeEnable_in_next 	<= '1';
															
														end if;
														
													end if;
												end if;
						end if;
						
						
					else 
					state_next				<= S_START;
					isWait_next				<= '0';
					swait_count_next		<=  0 ;
					iwait_count_next		<=  0 ;
					--wait_count_next			<= (others => '0');
				end if;
		
			end CASE;
	end process;

	process(isS6,uart_state,toSend_uart,uart_rx_enable,uart_rx_data,uart_tx_ready)
	begin
		uart_state_next <= uart_state;

		case uart_state.fsm_state is
			when idle =>
				if(isS6 = '1') then 
				uart_state_next.rx_done <= '0';
				uart_state_next.tx_done <= '0';
				else
					if(toSend_uart = '1' and uart_state.tx_done = '0') then
						--if(uart_tx_ready = '1')then 
							uart_state_next.tx_data <= sw_in;
							
							uart_state_next.fsm_state <= emitting;
						--end if;
					end if;
					
					if uart_rx_enable = '1' then
						--uart_state_next.tx_enable <= '0';
						uart_state_next.rx_data	<= uart_rx_data;
						uart_state_next.fsm_state <= received;
					end if;
				end if;
				
				
				
				
			when received =>
				--uart_state_next.tx_enable <= '0';
				uart_state_next.fsm_state <= idle;
				uart_state_next.rx_done <= '1';
				
			when emitting =>
				uart_state_next.tx_enable <= '1';
					if uart_tx_ready = '0' then
						uart_state_next.tx_enable <= '0';
						uart_state_next.fsm_state <= idle;
						uart_state_next.tx_done <= '1';
					end if;
		
		end case;
	end process;
	
	fsm_output: process (uart_state) is
	begin
		uart_tx_enable <= uart_state.tx_enable;
		uart_tx_data <= uart_state.tx_data;		
	end process;
	
	--process(chanAddr_in,f2hReady_in)
	--begin
	--	if(chanAddr_in /= "0000110" and f2hReady_in = '1')then 
	--		f2hValid_out 		<= '1';
	--	end if;
	--end process;


-- Drive register inputs for each channel when the host is writing

	-- Assert that there's always data for reading, and always room for writing
	f2hValid_out 		<= '1';
	h2fReady_out <= '1';                                                     --END_SNIPPET(registers)

	-- LEDs and 7-seg display
--	led_out <= reg0;
	flags <= "00" & f2hReady_in & reset_in;
	seven_seg : entity work.seven_seg
		port map(
			clk_in     => clk_in,
			data_in    => "0000000000000000",
			dots_in    => flags,
			segs_out   => sseg_out,
			anodes_out => anode_out
		);
end architecture;

--r6 4;w7 55777777;r6 4;w7 77887788;w7 b6fda0af;r6 4;w7 54b88589;r6 4;w7 77887788