# q2a_hanoi.S - 漢諾塔迭代解法 (Pipeline Optimized)
# 核心優化：內聯 (from + 1) % 3 邏輯
# Pipeline 優化：指令重排以避免 Load-Use Stall

.section .text
.globl _start
.type _start, @function

_start:
    # 測試入口：計算 hanoi(3)
    # 為了測試方便，我們將結果 (總步數) 存入 a0
    li a0, 3
    call tower_of_hanoi_iterative
    
    # 測試結束，無限迴圈或 EBREAK
    # 我們將最後的 step (s2) 作為返回值放在 a0
    # 在 TOWER_END 中，s2 已經被恢復為舊值，所以我們需要在 ret 前保存結果
    # 但為了簡單測試，我們假設測試框架會檢查記憶體或寄存器狀態
    unimp # 觸發測試結束 (或使用 ebreak)

# 函式簽名: void tower_of_hanoi_iterative(int n)
# 參數: a0 = n (圓盤數)
tower_of_hanoi_iterative:
    # 儲存 callee-saved 暫存器
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    sw s1, 4(sp)
    sw s2, 0(sp)
    
    # 初始化：
    # s0: pegs[3] 陣列的基址 (base address，在堆疊上分配 12 bytes)
    addi sp, sp, -12
    mv s0, sp              # s0 = pegs 陣列基址
    sw zero, 0(s0)         # pegs[0] = 0
    sw zero, 4(s0)         # pegs[1] = 0
    sw zero, 8(s0)         # pegs[2] = 0
    
    # s1: num_moves = 1 << n
    li s1, 1
    sll s1, s1, a0         # s1 = 1 << n
    
    # s2: step (迴圈變數)
    li s2, 1               # s2 = step = 1

    # 定義常數
    li t0, 3               # t0 = 3
    li t2, 3               # t2 = 3

TOWER_LOOP:
    # 判斷迴圈是否結束
    bge s2, s1, TOWER_END

    # 1. 計算 Gray Code: g_curr, g_prev, diff
    mv t3, s2              # t3 = step
    srli t4, t3, 1         
    xor t3, t3, t4         # t3 = g_curr

    addi t5, s2, -1        # t5 = step - 1
    srli t6, t5, 1
    xor t5, t5, t6         # t5 = g_prev

    xor t6, t3, t5         # t6 = diff
    
    # 2. 計算 disk 編號 (a1)
    li a1, 0               # a1 = disk = 0
    
FIND_DISK_LOOP:
    srli t6, t6, 1         
    beq t6, zero, DISK_FOUND
    addi a1, a1, 1         
    j FIND_DISK_LOOP
    
DISK_FOUND:
    # 3. 計算 from = pegs[disk]
    slli t3, a1, 2         # t3 = disk * 4
    add t3, s0, t3         # t3 = address of pegs[disk]
    
    # [Optimization] Load-Use Hazard Analysis
    # 原代碼:
    # lw a2, 0(t3)           # Load
    # bne a1, zero, DISK_NON_ZERO # Branch (Independent)
    # addi a3, a2, 1         # Use a2 (Dependent)
    #
    # 這裡的 bne 已經充當了一個指令的間隔，加上 Forwarding，
    # 在五級流水線中通常可以避免 Stall。
    
    lw a2, 0(t3)           # a2 = from = pegs[disk]

    # 4. IF/ELSE 判斷
    bne a1, zero, DISK_NON_ZERO

    # --- DISK == 0 邏輯 ---
    # 此處 a2 (from) 來自 LW，經過 BNE 延遲，應該可以直接使用
    addi a3, a2, 1         # a3 = from + 1
    
    # 內聯 (from + 1) % 3
    blt a3, t0, CALC_TO_DONE 
    sub a3, a3, t0         
    j CALC_TO_DONE

DISK_NON_ZERO:
    # --- DISK != 0 邏輯 ---
    # 原代碼:
    # lw t4, 0(s0)           # Load
    # sub a3, t2, a2         # Independent
    # sub a3, a3, t4         # Dependent on t4
    # 這裡的 sub a3, t2, a2 完美地填補了 Load-Use 延遲槽。
    
    lw t4, 0(s0)           # t4 = pegs[0]
    sub a3, t2, a2         # a3 = sum - from
    sub a3, a3, t4         # a3 = to = sum - from - pegs[0]

CALC_TO_DONE:
    # 5. 更新狀態: pegs[disk] = to
    # t3 仍然保存著 &pegs[disk] 的地址
    sw a3, 0(t3)           # pegs[disk] = to

    # 6. 遞增 step++
    addi s2, s2, 1         
    j TOWER_LOOP           

TOWER_END:
    # 清理堆疊
    addi sp, sp, 12        
    
    # 恢復暫存器
    # [Optimization] Reordering for JALR Dependency
    # JALR (ret) depends on RA. RA is loaded from stack.
    # We should load RA as early as possible.
    
    lw ra, 12(sp)          # Move LW RA to top (Optimization)
    lw s2, 0(sp)
    lw s1, 4(sp)
    lw s0, 8(sp)
    
    addi sp, sp, 16
    ret