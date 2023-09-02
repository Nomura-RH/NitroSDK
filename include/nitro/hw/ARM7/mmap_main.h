#ifndef NITRO_HW_ARM7_MMAP_MAIN_H_
#define NITRO_HW_ARM7_MMAP_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// MEMORY MAP of MAIN MEMORY
//---------------------------------------------------------------------------
//
//
//      Retail NITRO            Development NITRO 
//      +---------------+-------+---------------+--8.000M
//      | System Shared |       | System Shared |
//      +---------------+-------+---------------+
//      | User   Shared |       | User   Shared |
//      +---------------+-------+---------------+
//      | Sub Processor |       | Sub Processor |
//      | Reserved      |       | Reserved      |
//      +-+-------------+-------+---------------+--7.875M
//      |W|             |       |               |
//      |R|Main         |       | Debugger Work |
//      |A|Processor    |       |               |
//      |P|Reserved     |       +---------------+--7.000M
//      | |             |       |               |
//      |A|             |       | Main          |
//      |R|             |       | Processor     |
//      |O+-------------+--4M---+ Extra         +--4.000M
//      |U|System Shared|       | Reserved      |
//      |N+-------------+       |               |
//      |D|User   Shared|       |               |
//      | +-------------+       +               +
//      | |Sub Processor|       |               |
//      | |Reserved     |       |               |
//      +-+-------------+-------+---------------+--3.875M
//      |               |       |               |
//      | Main          |       | Main          |
//      | Processor     |       | Processor     |
//      | Reserved      |       | Reserved      |
//      |               |       |               |
//      |               |       |               |
//      |               |       |               |
//      +---------------+       +---------------+
//
//      Default setting for 4MB
//         MAIN  :3.875MB
//         SUB   :124KB
//         SHARED:  4KB
//         DTCM  :on top of SUB
//
#define HW_MAIN_MEM_SHARED_SIZE     0x001000

// HW_MAIN_MEM_MAIN_SIZE is able to change to these size,
// [ 0x200000, 0x300000, 0x380000, 0x3C0000, 0x3E0000, 0x3F0000, 0x3F8000, 0x3FC000 ]
#ifdef SDK_TS
#define HW_MAIN_MEM_MAIN_SIZE       0x3E0000
#else
#define HW_MAIN_MEM_MAIN_SIZE       0x380000
#endif
#define HW_MAIN_MEM_SUB_SIZE        (HW_MAIN_MEM_SIZE - HW_MAIN_MEM_MAIN_SIZE - HW_MAIN_MEM_SHARED_SIZE)

#define HW_MAIN_MEM_MAIN            (HW_MAIN_MEM)
#define HW_MAIN_MEM_MAIN_END        (HW_MAIN_MEM + HW_MAIN_MEM_MAIN_SIZE)
#define HW_MAIN_MEM_SUB             (HW_MAIN_MEM_MAIN_END + 0x400000)
#define HW_MAIN_MEM_SUB_END         (HW_MAIN_MEM_SUB + HW_MAIN_MEM_SUB_SIZE)

#define HW_MAIN_MEM_DEBUGGER_OFFSET 0x700000
#define HW_MAIN_MEM_DEBUGGER_SIZE   0x080000
#define HW_MAIN_MEM_DEBUGGER        (HW_MAIN_MEM + HW_MAIN_MEM_DEBUGGER_OFFSET)
#define HW_MAIN_MEM_DEBUGGER_END    (HW_MAIN_MEM_DEBUGGER + HW_MAIN_MEM_DEBUGGER_SIZE)

#define HW_MAIN_MEM_SHARED          (HW_MAIN_MEM_EX_END - HW_MAIN_MEM_SHARED_SIZE)
#define HW_MAIN_MEM_SHARED_END      (HW_MAIN_MEM_EX_END - HW_MAIN_MEM_SYSTEM_SIZE)

#define HW_MAIN_MEM_SYSTEM_END      (HW_MAIN_MEM_EX_END)

#ifdef __cplusplus
} /* extern "C" */
#endif
/* NITRO_HW_ARM7_MMAP_MAIN_H_ */
#endif
