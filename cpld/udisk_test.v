// udisk_test.v
// test code for udisk_cpld.v

`define debug 1
`include "udisk_cpld.v"

`timescale 1ns / 1ns

module udisk_test;

   wire [17:0] BUS_ADDR;
   wire [15:0] BUS_DATA;
   wire        BUS_DATA_DIR, BUS_ADDR_DIR;
   wire        BG4_OUT, BG5_OUT, NPG_OUT;
   wire        INTR_OUT, BR4_OUT, BR5_OUT, NPR_OUT;
   wire        MSYN_OUT, SSYN_OUT, BBSY_OUT;
   wire        C0_OUT, C1_OUT, SACK_OUT;
   wire [15:0] CPU_D;
   wire        CPU_INT, LED_OUT;
   wire        CF_CS0_N, CF_CS1_N, CF_IORD_N, CF_IOWR_N;

   reg 	       BBSY_IN, MSYN_IN, 
	       INIT_IN, BG4_IN, BG5_IN, NPG_IN, SACK_IN,
	       SSYN_IN, C0_IN, C1_IN,
	       CPU_RD, CPU_WR,
	       CPU_A0, CPU_A1, CPU_A2, CPU_A3;
   reg 	       RESET_L;

   reg 	       CLK;

   
   udisk_cpld cpld( BBSY_IN, MSYN_IN, BUS_ADDR, BUS_ADDR_DIR,
		    BUS_DATA, BUS_DATA_DIR,
		    PA_IN, PA_OUT, PB_IN, PB_OUT,
		    INIT_IN, BG4_IN, BG5_IN, NPG_IN, SACK_IN,
		    SSYN_IN, C0_IN, C1_IN,
		    BG4_OUT, BG5_OUT, NPG_OUT,
		    INTR_OUT, BR4_OUT, BR5_OUT, NPR_OUT,
		    MSYN_OUT, SSYN_OUT, BBSY_OUT,
		    C0_OUT, C1_OUT, SACK_OUT,
		    CPU_D, CPU_INT, CPU_RD, CPU_WR,
		    CPU_A0, CPU_A1, CPU_A2, CPU_A3,
		    LED_OUT,
		    CF_CS0_N, CF_CS1_N, CF_IORD_N, CF_IOWR_N,
		    RESET_L, CLK, DISK_RESET_N);

   reg [15:0] data;
   integer    timeout;

   initial
     begin
	$timeformat(-9, 0, "ns", 7);
	$dumpfile("udisk.vcd");
//	$dumpvars(0, udisk_test.cpld);
	$dumpvars(0, udisk_test);
     end

   task read_reg;
      input [3:0] r;
      output [15:0] data;
      begin
	 $display("read_reg %x ->", r);
	 { CPU_A3,CPU_A2,CPU_A1,CPU_A0 } = r;
	 CPU_RD = 1;
	 #5 ;
	 data = CPU_D;
	 #1 ;
	 $display("read_reg %x -> %x", r, CPU_D);
	 CPU_RD = 0;
      end
   endtask 
   
   task write_reg;
      input [3:0] r;
      input [15:0] data;
      begin
	 $display("write_reg %x <- %x", r, data);
	 force CPU_D = data;
	 { CPU_A3,CPU_A2,CPU_A1,CPU_A0 } = r;
	 CPU_WR = 1;
	 #5 ;
	 CPU_WR = 0;
	 #5 ;
	 release CPU_D;
      end
   endtask 
   
   task write_subreg;
      input [7:0] subreg;
      input [15:0] data;
      begin
	write_reg(4'b0111, { subreg, data[7:0] });
      end
   endtask

   task read_ide;
      input [3:0] r;
      output [15:0] data;
      begin
	 { CPU_A3,CPU_A2,CPU_A1,CPU_A0 } = 4'b1000 | { 1'b0, r[2:0] };
	 CPU_RD = 1;
	 #5 ;
	 CPU_RD = 0;
      end
   endtask 
   
   task write_ide;
      input [3:0] r;
      input [15:0] data;
      begin
	 { CPU_A3,CPU_A2,CPU_A1,CPU_A0 } = 4'b1000 | { 1'b0, r[2:0] };
	 force CPU_D = data;
	 CPU_WR = 1;
	 #10 ;
	 CPU_WR = 0;
	 #5 ;
	 release CPU_D;
      end
   endtask 
   
   task wait_for_ssyn;
      output timeout;
      integer loops, timeout;
      begin
	 loops = 0;
	 timeout = 0;
	 $display("wait for ssyn; %t", $time);
	 while (SSYN_OUT == 0 && timeout == 0)
	   begin
	      @(posedge CLK);
	      loops = loops + 1;
	      if (loops > 100)
		begin
		   $display("TIMEOUT waiting for ssyn; %t", $time);
		   //failure(100);
		   timeout = 1;
		end
	   end
	 if (timeout == 0)
	   begin
	      $display("got ssyn; %t", $time);
	      $display("got ssyn; BUS_DATA %x", BUS_DATA);
	   end
      end
   endtask

   task wait_for_msyn;
      output timeout;
      integer loops, timeout;
      begin
	 loops = 0;
	 timeout = 0;
	 // wait for MSYN to assert
	 $display("wait for msyn; %t", $time);
	 while (MSYN_OUT == 0 && timeout == 0)
	   begin
	      @(posedge CLK);
	      loops = loops + 1;
	      if (loops > 100)
		begin
		   $display("TIMEOUT waiting for msyn; %t", $time);
		   timeout = 1;
		end
	   end
	 if (timeout == 0)
	   begin
	      $display("got msyn; %t", $time);
	      $display("got msyn; BUS_DATA %x", BUS_DATA);
	   end
	 // assert SSYN, wait for MSYN to deassert
	 loops = 0;
	 SSYN_IN = 1;
	 while (MSYN_OUT == 1 && timeout == 0)
	   begin
	      @(posedge CLK);
	      loops = loops + 1;
	      if (loops > 100)
		begin
		   $display("TIMEOUT waiting for ~msyn; %t", $time);
		   timeout = 1;
		end
	   end
	 SSYN_IN = 0;
	 if (timeout == 0)
	   begin
	      $display("got ~msyn; %t", $time);
	   end
// need to make sure master state machine finishes
	 #10 ;
      end
   endtask

   task failure;
      input f;
      integer f;
      begin
	 $display("FAILURE %d", f);
	 $finish;
      end
   endtask

   task success;
      begin
	 $display("SUCCESS!");
	 #100 $finish;
      end
   endtask // success

   always
     begin
	#1 CLK = 0;
	#1 CLK = 1;
     end

   initial
     begin
	BBSY_IN = 0;
	MSYN_IN = 0;
	INIT_IN = 0;
	BG4_IN = 0;
	BG5_IN = 0;
	NPG_IN = 0;
	SACK_IN = 0;
	SSYN_IN = 0;
	C0_IN = 0;
	C1_IN = 0;

	CPU_RD = 0;
	CPU_WR = 0;
	{ CPU_A3,CPU_A2,CPU_A1,CPU_A0 } = 4'b0;

	RESET_L = 0;

	// ---------------- assert reset ------------------
	#1 begin
	   RESET_L = 0;
	   force BUS_ADDR = 18'h00000;
	   //force CPU_D = 16'h0000;
        end

	#10 begin
	   RESET_L = 1;
	end

	// ---------------- setup - write & read registers ------------------
	// read ID register
	#10 ;

	read_reg(4'b0111, data);
	if (data != 16'ha55a) failure(1);

	read_reg(4'b0111, data);
	if (data != 16'ha55a) failure(1);

	read_reg(4'b0111, data);
	if (data != 16'ha55a) failure(1);

	// write assert register
	write_reg(4'b0001, 16'b0);

	// write assert register - INTR=1
	write_reg(4'b0001, 16'h0001);
	if (INTR_OUT != 1) failure(2);

	// write assert register - INTR=0
	write_reg(4'b0001, 16'b0000);
	if (INTR_OUT != 0) failure(2);

	// write assert register - SSYN=1
	write_reg(4'b0001, 16'h0200);
	if (SSYN_OUT != 1) failure(2);

	// write assert register - SSYN=0
	write_reg(4'b0001, 16'h0000);
	if (SSYN_OUT != 0) failure(2);

	// check all the unibus lines
	write_reg(4'b0001, 16'h0001);
	if (INTR_OUT != 1) failure(2);

	write_reg(4'b0001, 16'h0002);
	if (BR4_OUT != 1) failure(3);

	write_reg(4'b0001, 16'h0004);
	if (BR5_OUT != 1) failure(3);

	write_reg(4'b0001, 16'h0008);
	if (NPR_OUT != 1) failure(3);

	write_reg(4'b0001, 16'h0100);
	if (MSYN_OUT != 1) failure(3);

	write_reg(4'b0001, 16'h0200);
	if (SSYN_OUT != 1) failure(3);

	write_reg(4'b0001, 16'h0400);
	if (BBSY_OUT != 1) failure(3);

	write_reg(4'b0001, 16'h0800);
	if (C0_OUT != 1) failure(3);

	write_reg(4'b0001, 16'h1000);
	if (C1_OUT != 1) failure(3);

	write_reg(4'b0001, 16'h2000);
	if (SACK_OUT != 1) failure(3);

	write_reg(4'b0001, 16'h4000);
	if (BUS_DATA_DIR != 1) failure(4);

	write_reg(4'b0001, 16'h8000);
	if (BUS_ADDR_DIR != 1) failure(4);

	write_reg(4'b0001, 16'h0000);

	// ---------------- test bus data path ------------------
	$display("\n----- bus data path; %t -----", $time);

	#10 ;
	// drive data bus out
	write_reg(4'b0001, 16'h4000);
	write_reg(4'b0110, 16'h1234);
	$display("test bus data; BUS_DATA %x", BUS_DATA);
	if (BUS_DATA != 16'h1234) failure(5);
	// release data bus
	write_reg(4'b0001, 16'h0000);

`ifdef broken
	#10 ;
	force BUS_DATA = 16'h4321;
	write_reg(4'b0001, 16'h0000);
	read_reg(4'b0110, data);
	$display("read bus data; BUS_DATA %x", BUS_DATA);
	$display("read bus data; data %x", data);
	if (data != 16'h4321) failure(6);
	release BUS_DATA;
`endif

	#10 ;
	write_reg(4'b0110, 16'ha500);
	read_reg(4'b0110, data);
	$display("read bus pattern; data %x", data);
	if (data != 16'ha500) failure(7);

	write_reg(4'b0110, 16'h005a);
	read_reg(4'b0110, data);
	$display("read bus pattern; data %x", data);
	if (data != 16'h005a) failure(7);

	// ---------------- test bus address path ------------------
	$display("\n----- test bus address path; %t -----", $time);

	release BUS_ADDR;
	
	// write addr_out
	write_reg(4'b0100, 16'h0003);
	write_reg(4'b0101, 16'h1234);

	// write assert to set bus addr dir
	write_reg(4'b0001, 16'h8000);
	if (BUS_ADDR_DIR != 1) failure(8);
	$display("test bus addr; BUS_ADDR %x", BUS_ADDR);
	if (BUS_ADDR != 18'h31234) failure(9);
	
	// deassert bus addr dir
	write_reg(4'b0001, 16'h0000);

	// ------------ unibus access - address match, all ones -----------
	$display("\n---- unibus - address match, all ones; %t -----", $time);
	
	// write addr_match_1
	write_reg(4'b0010, 16'hffff);

	// write addr_mask_1
	write_subreg(8'h05, 16'hffff);

	// fake a cpu write transaction with matching address
	#2 ;
	BBSY_IN = 1;
	MSYN_IN = 1;
	C1_IN = 1;
	
	force BUS_ADDR = 18'h3ffff;
	force BUS_DATA = 16'h3456;

	// do cpu release
	write_subreg(8'h05, 16'h0000);

	// wait for syn
	wait_for_ssyn(timeout);
	if (timeout != 0) failure(10);
	
	#10 ;
	BBSY_IN = 0;
	MSYN_IN = 0;
	#2 C1_IN = 0;

	// check data (make sure state machine ran)
	read_reg(4'b0110, data);
	$display("addr match; data %x", data);
	if (data != 16'h3456) failure(10);

	// then clear match
	#2 ;
	write_reg(4'b0010, 16'h0000);
	release BUS_ADDR;
	release BUS_DATA;

	// ------------ unibus access - address non-match -----------
	$display("\n---- unibus - address non-match; %t -----", $time);

	// fake a cpu write transaction with matching address
	#2 ;
	BBSY_IN = 1;
	MSYN_IN = 1;
	force BUS_ADDR = 18'h0ffff;
	force BUS_DATA = 16'h0000;
	C1_IN = 1;

	// wait for syn
	wait_for_ssyn(timeout);
	if (timeout == 0) failure(11);
	
	#10 ;
	BBSY_IN = 0;
	MSYN_IN = 0;
	#2 C1_IN = 0;

	// then clear match
	#2 ;
	write_reg(4'b0010, 16'h0000);
	release BUS_ADDR;
	release BUS_DATA;

	// ------------ unibus access - address match, mixed -----------
	$display("\n---- unibus - address match, cpu write; %t -----", $time);

	// fake a cpu write transaction with matching address
	// write addr_match_1
	#10 ;
	write_reg(4'b0010, 16'h8888);

	// write addr_mask_1
	write_subreg(8'h05, 16'hffff);

	#2 ;
	BBSY_IN = 1;
	MSYN_IN = 1;
	force BUS_ADDR = 18'h22220;
	force BUS_DATA = 16'h0000;
	C1_IN = 1;

	// do cpu release
	#2 ;
	write_subreg(8'h05, 16'h0000);

	// wait for syn
	wait_for_ssyn(timeout);
	if (timeout != 0) failure(12);

	#10 ;
	BBSY_IN = 0;
	MSYN_IN = 0;
	#2 C1_IN = 0;

	read_reg(4'b0110, data);
	$display("addr match, mixed, cpu write; data %x", data);
	if (data != 16'h0000) failure(12);

	// then clear match
	#2 ;
	write_reg(4'b0010, 16'h0000);
	release BUS_ADDR;
	release BUS_DATA;

	// ------------ unibus access - address match, mixed -----------
	$display("\n---- unibus - address match, cpu read; %t -----", $time);

	// fake a cpu read transaction with matching address
	// write addr_match_1
	#10 ;
	write_reg(4'b0010, 16'h8888);

	// write addr_mask_1
	write_subreg(8'h05, 16'hffff);

	#2 ;
	force BUS_ADDR = 18'h22220;
	BBSY_IN = 1;
	MSYN_IN = 1;

	// do cpu release
	#2 ;
	// data to present to cpu
	write_reg(4'b0110, 16'h5678);
	// cpu_release
	write_subreg(8'h05, 16'h0000);

	// wait for syn
	wait_for_ssyn(timeout);
	if (timeout != 0) failure(13);
	
	read_reg(4'b0110, data);
	$display("addr match, mixed, cpu read; data %x", data);
	if (data !== 16'h5678) failure(13);

	#10 ;
	BBSY_IN = 0;
	MSYN_IN = 0;

	// then clear match
	#2 ;
	write_reg(4'b0010, 16'h0000);
	release BUS_ADDR;

	// ------------ unibus access - dma moded -----------
	$display("\n---- unibus - dma mode; %t -----", $time);

	// dma_mode
	write_subreg(8'h04, 16'h0001);

	// write addr_out
	write_reg(4'b0100, 16'h0003);
	write_reg(4'b0101, 16'h1234);

	// write assert to set bus addr dir
	write_reg(4'b0001, 16'hc000);

	write_reg(4'b0110, 16'h1111);
	wait_for_msyn(timeout);

	write_reg(4'b0110, 16'h2222);
	wait_for_msyn(timeout);
	
	write_reg(4'b0110, 16'h3333);
	wait_for_msyn(timeout);
	
	// clean dma_mode
	write_subreg(8'h04, 16'h0000);

$finish;
	
`ifdef not_now
	// ------------ unibus access - address match, all ones -----------

	// write addr_match_2
	#10 ;
	write_reg(4'b0011, 16'hffff);

	// write addr_mask_2
	write_subreg(8'h07, 16'hffff);

	// fake a bus transaction with matching address
	#2 ;
	BBSY_IN = 1;
	MSYN_IN = 1;
	force BUS_ADDR = 18'h3ffff;

	// wait for syn
	wait_for_ssyn(timeout);
	if (timeout != 0) failure(14);
	
	#10 ;
	BBSY_IN = 0;
	MSYN_IN = 0;

	// then clear match
	#2 ;
	write_reg(4'b0010, 16'h0000);
	release BUS_ADDR;
`endif
	
	// ------------------------- ide access ----------------------
	$display("\n---- ide access; %t -----", $time);

	release CPU_D;
	
	// write cf_enable registers
	write_subreg(8'h03, 16'h0001);

	// write ide
	write_ide(4'b1100, 16'h0001);
      
	// write ide
	write_ide(4'b1100, 16'h0002);
      
	// write ide
	write_ide(4'b1100, 16'h00ff);
      
	// read ide
	#10 ;
	force CPU_D = 16'h0000;
	read_ide(4'b1101, data);
	read_ide(4'b1101, data);
	if (data != 16'h0000) failure(21);
	
	// read ide
	force CPU_D = 16'h1234;
	read_ide(4'b1101, data);
	if (data != 16'h1234) failure(22);
      
	// read ide
	force CPU_D = 16'h4321;
	read_ide(4'b1101, data);
	if (data != 16'h4321) failure(23);

	// clear cf_enable registers
	write_subreg(8'h03, 16'h0000);
      
	release CPU_D;

	// write ide
	write_ide(4'b1100, 16'h00ff);
      
	// read ide
	force CPU_D = 16'h00ff;
	read_ide(4'b1101, data);


	// ----- done -----
	
	success;

     end

endmodule
