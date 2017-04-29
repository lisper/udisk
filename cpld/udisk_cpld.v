// UDISK CPLD - original bus method - minimal
// XCR3256-144
// $Id$

//======================================================================
// CPU
//
// cpu_addr - from arm
// 0000	write pass-thru
// 0001	write assert pins		read status
// 0010	write addr match 1 addr		read match addr hi
// 0011	write addr match 1 mask		read match addr lo
// 0100	write addr buffer hi
// 0101	write addr buffer lo
// 0110	write data buffer		read bus data
// 0111	subreg				id
//      01	write cpu int reset
//      02	write LED
//      03	write cf enable/reset
//      04	dma mode
//      05	cpu release
//
   
module udisk_cpld( BBSY_IN, MSYN_IN, BUS_ADDR, BUS_ADDR_DIR,
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

   // Input
   input BBSY_IN, MSYN_IN, SSYN_IN, C0_IN, C1_IN;
   input INIT_IN, BG4_IN, BG5_IN, NPG_IN, SACK_IN;
   input CPU_RD, CPU_WR, CPU_A0, CPU_A1, CPU_A2, CPU_A3, RESET_L;
   input PA_IN, PB_IN;
   input CLK;

   // Output
   output BG4_OUT, BG5_OUT, NPG_OUT;
   output INTR_OUT, BR4_OUT, BR5_OUT, NPR_OUT;
   output MSYN_OUT, SSYN_OUT, BBSY_OUT, C0_OUT, C1_OUT, SACK_OUT;
   output CPU_INT;
   output BUS_ADDR_DIR, BUS_DATA_DIR;
   output LED_OUT;
   output CF_CS0_N, CF_CS1_N, CF_IORD_N, CF_IOWR_N;
   output PA_OUT, PB_OUT;
   output DISK_RESET_N;

   // Bidirectional
   inout [17:0] BUS_ADDR;
   inout [15:0] BUS_DATA;
   inout [15:0] CPU_D;

   reg 		LED_OUT;

   // -------

   wire		reset;
 		
   reg [15:0] 	bus_data_hold;
   wire [15:0] 	data_mux;

   wire [3:0] 	cpu_addr;

   reg [3:0] 	pass_thru;
   reg [15:0] 	assert_pin;

   reg [1:0] 	cf_enable;
   reg 		disk_reset;

   reg [17:2] 	addr_match_1;
   reg [17:0] 	addr_out;

   wire 	addr_match;
   wire 	have_match1;

   reg 		cpu_int_internal;

   wire 	addr_strobe;
   wire 	addr_incr;
   wire 	cpu_int_assert;
   wire 	data_assert;
   wire 	data_strobe;
   wire 	ssyn_assert;
   wire		addr_assert;
   wire		c1_assert;
   wire		msyn_assert;
   wire 	unibus_master;
   reg 		cpu_release;
   
   reg		dma_mode;
   reg		pending_dma_write;
   reg		pending_dma_read;
   wire		pending_dma;

   reg [2:0] 	ustate_s;
   wire 	ustate_s_start;
   wire 	SS0, SS1, SS2, SS3;
   wire 	ustate_SS2_R;
   wire 	ustate_SS1_W;

   reg [5:0] 	ustate_m;
   wire 	ustate_m_start;
   wire 	MS0, MS1, MS2, MS3, MS4, MS5, MS6;

   parameter A_INTR_OUT = 0;
   parameter A_BR4_OUT = 1;
   parameter A_BR5_OUT = 2;
   parameter A_NPR_OUT = 3;
   parameter A_MSYN_OUT = 8;
   parameter A_SSYN_OUT = 9;
   parameter A_BBSY_OUT = 10;
   parameter A_C0_OUT = 11;
   parameter A_C1_OUT = 12;
   parameter A_SACK_OUT = 13;
   parameter A_BUS_DATA_DIR = 14;
   parameter A_BUS_ADDR_DIR = 15;
   
   // signals asserted on unibus (via 8641, which inverts to _L signals)
   assign    INTR_OUT = assert_pin[0];
   assign    BR4_OUT  = assert_pin[1];
   assign    BR5_OUT  = assert_pin[2];
   assign    NPR_OUT  = assert_pin[3];

   // unibus grant "pass through" enable
   assign    BG4_OUT = pass_thru[1] ? BG4_IN : assert_pin[4];
   assign    BG5_OUT = pass_thru[2] ? BG5_IN : assert_pin[5];
   assign    NPG_OUT = pass_thru[3] ? NPG_IN : assert_pin[6];

   // signals used by cpu when unibus bus master
   assign    MSYN_OUT  = assert_pin[8] | msyn_assert;
   assign    SSYN_OUT  = assert_pin[9] | ssyn_assert;
   assign    BBSY_OUT  = assert_pin[10];
   assign    C0_OUT    = assert_pin[11];
   assign    C1_OUT    = assert_pin[12] | c1_assert;
   assign    SACK_OUT  = assert_pin[13];

   assign    BUS_DATA_DIR = assert_pin[14] | data_assert;
   assign    BUS_ADDR_DIR = assert_pin[15] | addr_assert;

   // for now, these are dormant
   assign    PA_OUT = 0;
   assign    PB_OUT = 0;

   assign    cpu_addr = {CPU_A3, CPU_A2, CPU_A1, CPU_A0};

   // bidir cpu data bus - assert if cpu reads and it's not an ide access
   assign    CPU_D = (CPU_RD & CF_CS0_N & CF_CS1_N) ? data_mux : 16'bz;

   // bidir unibus address bus
   assign    BUS_ADDR = BUS_ADDR_DIR ? addr_out : 18'bz;

   // bidir unibus data bus
   assign    BUS_DATA = BUS_DATA_DIR ? bus_data_hold : 16'bz;

   // internal decode
   wire      DATI, DATIP, DATO, DATOB;

   assign    DATI =  C1_IN == 1'b0 && C0_IN == 1'b0;
   assign    DATIP = C1_IN == 1'b0 && C0_IN == 1'b1;
   assign    DATO =  C1_IN == 1'b1 && C0_IN == 1'b0;
   assign    DATOB = C1_IN == 1'b1 && C0_IN == 1'b1;

   // reset
   assign reset = ~RESET_L || (CPU_RD && CPU_WR);
			       
// Slave states:
//
// SS0 idle
// wait until (BBSY_IN && MSYN_IN && addr match)
// 
// SS1_R if (DATI || DATIP)		unibus master read
//     read
//	latch bus address
//	assert cpu_int
//      wait for cpu_release
//      next_ustate = SS2_R
//
// SS2_R
//	assert DATA
//	assert SSYN_OUT
//      wait for MSYN_IN == 0
//      next_ustate = SS0
//
// SS1_W if (DATO || DATOB)		unibus master write
//     write
//	latch bus address
//	latch bus data
//	assert cpu_int
//      wait for cpu_release
//      next_ustate = SS2_W
//
// SS2_W
//	assert SSYN_OUT
//      wait for MSYN_IN == 0
//      next_ustate = SS0
//

// Master states:
//
// ** BG* asserts high!
// ** NPG asserts high!
// arbitration is separate from taking the bus
// assert NPR or BR*, then assert SSACK
// then wait for no BBSY, then assert BBSY

// burst start
// BS1
//	assert NPG (for pass-thru, this makes it low on bus)
//	disable pass-thru
//	assert NPR
//	wait for NPG
// BS2
//	assert SACK (5-10us timeout)
//	deassert NPR (must be same time as SACK deassert)
//	wait for no NPG
// BS3
//	wait for no BBSY
//	wait for no SSYN
// BS4
//	assert BBSY
//	deassert SACK (slightly after BBSY deassert)
//
// BE1 (software)
//      enable NPG pass-thru
//	deassert BBSY
//	deassert NPG

//
// burst read/write
// MS1
//	assert C1 if write
//	assert data if write
//	assert address
//      next_ustate = MS2
//
// MS2
// 	wait 75ns
//      next_ustate = MS3
//
// MS3
// 	wait 75ns
//	assert MSYN
//      next_ustate = MS4
//	
// MS4
//	assert MSYN
//	wait for SSYN
//	sample data if read
//      next_ustate = MS5
//	
// MS5
//	deassert MSYN
//	wait for ~SSYN
//	deassert data if write
//	deassert C1 if write
//      next_ustate = MS6
//
// MS6
//	wait 75ns
//	deassert address
//      next_ustate = SS0

// cpu master interrupt (software)
//	assert NPG
//	disable BRx pass-thru
//	
//	 assert BRx
//	 wait for BGx
//	
//	 assert SACK (5-10us timeout)
//	 deassert BRx (must be same time as SACK deassert)
//	 wait for no BGx
//	
//	 wait for no BBSY
//	 wait for no SSYN
//	
//	 assert BBSY
//	 assert data (with vector)
//	 wait for no SSYN
//	
//	 assert INTR
//	 deassert SACK (slightly after INTR assert)
//	 wait for SSYN
//	
//	 deassert data
//	 deassert INTR
//	 deassert BBSY
//	
//	enable pass-thru
//	deassert NPG
//

   reg ssyn, msyn, bbsy;

   always @(posedge CLK)
     if (reset)
       begin
	  ssyn <= 0;
	  msyn <= 0;
	  bbsy <= 0;
       end
     else
       begin
	  ssyn <= SSYN_IN;
	  msyn <= MSYN_IN;
	  bbsy <= BBSY_IN;
       end
   
   // slave states
   assign SS0 = ~|ustate_s;
   assign SS1 = ustate_s[0];
   assign SS2 = ustate_s[1];
   assign SS3 = ustate_s[2];
   
   assign unibus_master = bbsy & msyn & addr_match;

   assign ustate_s_start = unibus_master && SS0 && MS0 && ~dma_mode;
   
   always @(posedge CLK)
     if (reset)
       ustate_s <= 0;
     else
       if ((SS1 && ~cpu_release) ||
	   (SS3 && unibus_master))
	 ustate_s <= ustate_s;
       else
	 ustate_s <= { ustate_s[1:0], ustate_s_start };
   
   // slave
   assign ustate_SS1_W = (DATO || DATOB) && SS1;
   assign ustate_SS2_R = (DATI || DATIP) && (SS2 || SS3);

   assign ssyn_assert = SS3;

   /* assert int at transition from SS0 -> SS1 */
   assign cpu_int_assert = SS1;

   assign data_strobe = ustate_SS1_W || (MS4 && pending_dma_read);

   assign addr_strobe = SS1 && msyn;

//   assign next_ustate =
//	/* unibus slave read/write */
//	(ustate == SS0 && unibus_master) ? SS1 :
//	(ustate == SS1 && unibus_master && !cpu_release) ? SS1 :
//	(ustate == SS1 && unibus_master && cpu_release) ? SS2 :
//    	(ustate == SS2 && unibus_master) ? SS2 :
//	(ustate == SS2 && !unibus_master) ? SS0 :
//	/* ubibus master read/write */
//	(ustate == SS0 && !unibus_master && pending_dma) ? MS1 :
//	(ustate == MS1) ? MS2 :
//      	(ustate == MS2) ? MS3 :
//      	(ustate == MS3) ? MS4 :
//       	(ustate == MS4 && !SSYN_IN) ? MS4 :
//      	(ustate == MS4 && SSYN_IN) ? MS5 :
//      	(ustate == MS5 && SSYN_IN) ? MS5 :
//      	(ustate == MS5 && ~SSYN_IN) ? MS6 :
//      	(ustate == MS6) ? SS0 :
//	SS0;

   // master states
   assign MS0 = ~|ustate_m;
   assign MS1 = ustate_m[0];
   assign MS2 = ustate_m[1];
   assign MS3 = ustate_m[2];
   assign MS4 = ustate_m[3];
   assign MS5 = ustate_m[4];
   assign MS6 = ustate_m[5];

   assign ustate_m_start = (~unibus_master && pending_dma) && SS0 && MS0;

   wire [5:0] ustate_m_next;

   assign ustate_m_next =
	  ~dma_mode ? 6'b0 :  				// to reset hung xfer
	  ((MS4 && ~ssyn) || (MS5 && ssyn)) ? ustate_m :// stall waiting on ssyn
	  { ustate_m[4:0], ustate_m_start };		// advance

   always @(posedge CLK)
     if (reset)
       ustate_m <= 0;
     else
       ustate_m <= ustate_m_next;
   
//   reg ms4_d;
//   always @(posedge CLK)
//     if (reset) ms4_d <= 0;
//     else
//       ms4_d <= MS4;
   
   // start action as bus master
   assign pending_dma = dma_mode && (pending_dma_write || pending_dma_read);

   // master
   assign c1_assert = (MS1 || MS2 || MS3 || MS4) && pending_dma_write;

   assign msyn_assert = MS3 || MS4/* || ms4_d*/;

   assign addr_assert = MS1 || MS2 || MS3 || MS4 || MS5;

   assign addr_incr = MS6;

   // slave & master
   assign data_assert = ustate_SS2_R ||
			((MS1 || MS2 || MS3 || MS4) && pending_dma_write);

   // watch for address matches on UNIBUS

   assign have_match1 = (BUS_ADDR[17:6] == addr_match_1[17:6]) ?
			1'b1 : 1'b0;

   assign addr_match = have_match1;

   // edge detect for CPU_WR
   wire      cpu_wr_edge;
   reg [1:0] cpu_wr_history;
   
   assign cpu_wr_edge = ~cpu_wr_history[1] && cpu_wr_history[0];

   always @(posedge CLK)
     if (reset)
       cpu_wr_history <= 0;
     else
       cpu_wr_history <= { cpu_wr_history[0], CPU_WR };
   
   wire      cpu_rd_edge;
   reg [1:0] cpu_rd_history;
   
   // edge detect for CPU_RD
   assign cpu_rd_edge = ~cpu_rd_history[1] && cpu_rd_history[0];

   always @(posedge CLK)
     if (reset)
       cpu_rd_history <= 0;
     else
       cpu_rd_history <= { cpu_rd_history[0], CPU_RD };
   
   // pending dma write; cpu writes and master state
   always @(posedge CLK)
     if (reset)
	  pending_dma_write <= 0;
     else
       begin
          if (/*CPU_WR*/cpu_wr_edge && cpu_addr == 4'b0110)
	    pending_dma_write <= dma_mode ? 1'b1 : 1'b0;
	  else
	    if (MS6)
	      pending_dma_write <= 0;
       end

   // pending dma read; cpu reads and master state
   always @(posedge CLK)
     if (reset)
	  pending_dma_read <= 0;
     else
       begin
          if (cpu_rd_edge/*CPU_RD*/ && cpu_addr == 4'b0110)
	    pending_dma_read <= dma_mode ? 1'b1 : 1'b0;
	  else
	    if (MS6)
	      pending_dma_read <= 0;
       end

   // cpu writes to address registers
   always @(posedge CLK)
     if (reset)
       addr_out <= 0;
     else
       begin
	  if (addr_strobe)
	    addr_out <= BUS_ADDR;
	  else
	    if (/*CPU_WR*/cpu_wr_edge)
              begin
		 if (cpu_addr == 4'b0100)
		   begin
		      addr_out[17:16] <= CPU_D[1:0];
`ifdef debug
		      $display("hi addr_out %x", CPU_D[1:0]);
`endif
		   end
		 if (cpu_addr == 4'b0101)
		   begin
		      addr_out[15:0] <= CPU_D;
`ifdef debug
			$display("lo addr_out %x", CPU_D);
`endif
		   end
              end
	    else
	      if (addr_incr)
		begin
		   addr_out <= addr_out + 2;
`ifdef debug
		   $display("addr_incr %x", addr_out + 2);
`endif
		end
       end

   always @(posedge CLK)
     if (reset)
       bus_data_hold <= 0;
     else
       begin
	  if (data_strobe)
	    bus_data_hold <= BUS_DATA;
	  else
            if (/*CPU_WR*/cpu_wr_edge && cpu_addr == 4'b0110)
	      begin
		 bus_data_hold <= CPU_D;
`ifdef debug
		 $display("bus_data_hold %x, dir %b, bus %x; %t",
			  CPU_D, BUS_DATA_DIR, BUS_DATA, $time);
`endif
	      end
       end

   always @(posedge CLK)
     if (reset)
       begin
	  assert_pin <= 0;
	  pass_thru <= 0;
	  LED_OUT <= 0;
	  dma_mode <= 0;
	  addr_match_1 <= 0;
	  cpu_release <= 0;
       end
     else
       if (/*CPU_WR*/cpu_wr_edge)
	 begin
	    casex ( cpu_addr )
	      4'b0000: pass_thru <= CPU_D[3:0];
	      4'b0001:
		begin
		   assert_pin <= CPU_D;
`ifdef debug_assert
		   $display("assert_pin %b", CPU_D);
`endif
		end
	      4'b0010: addr_match_1[17:2] <= CPU_D;

	      4'b0111:
		begin
		   casex (CPU_D[15:8])
		     8'h01: ;
		     8'h02: LED_OUT <= CPU_D[0];
		     8'h03: ;
		     8'h04: dma_mode <= CPU_D[0];
		     8'h05: cpu_release <= 1;
		   endcase
		end
	    endcase
	 end 
       else
	    if (SS0 || SS2 || SS3)
	      cpu_release <= 0;

   wire [15:0] reg_status;

   assign reg_status = { CPU_INT, pending_dma, have_match1, MS0,
			 MS4|MS3|MS2|MS1, ustate_s,
			 //
			 BBSY_IN, C1_IN, C0_IN, SACK_IN,
			 BG4_IN, BG5_IN, NPG_IN, INIT_IN };
   
   assign data_mux =
		    cpu_addr == 4'b0000 ? { C1_IN, C0_IN, addr_out[5:0] } :
		    cpu_addr == 4'b0001 ? reg_status :
		    cpu_addr == 4'b0010 ? { ustate_m/*6'b000000*/, addr_out[17:16] } :
		    cpu_addr == 4'b0011 ? addr_out[15:0] :
		    cpu_addr == 4'b0110 ? bus_data_hold :
		    cpu_addr == 4'b0111 ? 16'ha5a3 :
		    0;

   // watch for address matches on UNIBUS

   assign have_match1 = (BUS_ADDR[17:6] == addr_match_1[17:6]) ?
			1'b1 : 1'b0;

   assign addr_match = have_match1;

   wire   cpu_wr_sub;
   assign cpu_wr_sub = /*CPU_WR*/cpu_wr_edge && cpu_addr == 4'b0111;
   

//======================================================================
// CF

   // special case resetting cf_enable - it always works
   always @(posedge CLK)
     if (reset)
       begin
	  cf_enable <= 2'b0;
	  disk_reset <= 1'b0;
       end
     else
       begin
	  if (cpu_wr_sub && (CPU_D[15:8] == 8'h03))
	    begin
	       disk_reset <= CPU_D[2];
	       cf_enable <= CPU_D[1:0];
	    end
       end

   // CF signals - only active if A3 == 1
   assign CF_CS0_N = ~(cf_enable[0] & CPU_A3);
   assign CF_CS1_N = ~(cf_enable[1] & CPU_A3);

   // CF/IDE reset
   assign DISK_RESET_N = ~disk_reset;

   assign CF_IORD_N = ~(CPU_RD==1 & (CF_CS1_N==0 | CF_CS0_N==0));
   assign CF_IOWR_N = ~(CPU_WR==1 & (CF_CS1_N==0 | CF_CS0_N==0));

//======================================================================
// CPU INTERRUPT

   wire cpu_int_reset_write;
   wire cpu_int_reset;

   // decode write to cpu int reset register
   assign cpu_int_reset_write = cpu_wr_sub && (CPU_D[15:8] == 8'h01);
   assign cpu_int_reset = cpu_int_reset_write | reset;

   always @(posedge CLK or posedge cpu_int_reset)
     if (cpu_int_reset)
       cpu_int_internal <= 0;
     else
       begin
	  cpu_int_internal <= cpu_int_assert;

`ifdef debug
	  if (cpu_int_assert)
	    $display("** cpu_int=1");
`endif
       end // else: !if(cpu_int_reset)
   
   // only assert int if A3 = 0 (locks out ints during CF access)
   assign CPU_INT = cpu_int_internal & ~CPU_A3;
   
endmodule // udisk_cpld
