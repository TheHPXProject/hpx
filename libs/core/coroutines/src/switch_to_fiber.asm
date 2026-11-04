;  Copyright (c) 2007-2026 Hartmut Kaiser
;
;  SPDX-License-Identifier: BSL-1.0
;
;  Distributed under the Boost Software License, Version 1.0.
;  (See accompanying file LICENSE_1_0.txt or copy at
;  http://www.boost.org/LICENSE_1_0.txt)

PUBLIC switch_to_fiber

_TEXT SEGMENT
switch_to_fiber proc
;
    mov         rdx,qword ptr gs:[30h]
    mov         rax,qword ptr [rdx+20h]
    mov         r8,qword ptr [rcx+20h]
    mov         qword ptr [rdx+1478h],r8
    mov         qword ptr [rdx+20h],rcx
    mov         r8,qword ptr [rdx+10h]
    mov         qword ptr [rax+18h],r8
    mov         r8d,dword ptr [rdx+1748h]
    mov         dword ptr [rax+518h],r8d
    mov         r8,qword ptr [rdx+17C8h]
    mov         qword ptr [rax+510h],r8
    mov         r8,qword ptr [rdx+2C8h]
    mov         qword ptr [rax+508h],r8
    lea         r8,[rax+30h]
    mov         qword ptr [r8+90h],rbx
    mov         qword ptr [r8+0A0h],rbp
    mov         qword ptr [r8+0A8h],rsi
    mov         qword ptr [r8+0B0h],rdi
    mov         qword ptr [r8+0D8h],r12
    mov         qword ptr [r8+0E0h],r13
    mov         qword ptr [r8+0E8h],r14
    mov         qword ptr [r8+0F0h],r15
    movaps      xmmword ptr [r8+200h],xmm6
    movaps      xmmword ptr [r8+210h],xmm7
    movaps      xmmword ptr [r8+220h],xmm8
    movaps      xmmword ptr [r8+230h],xmm9
    movaps      xmmword ptr [r8+240h],xmm10
    movaps      xmmword ptr [r8+250h],xmm11
    movaps      xmmword ptr [r8+260h],xmm12
    movaps      xmmword ptr [r8+270h],xmm13
    movaps      xmmword ptr [r8+280h],xmm14
    movaps      xmmword ptr [r8+290h],xmm15
    stmxcsr     dword ptr [r8+34h]
    fnclex
    wait
    fnstcw      word ptr [r8+100h]
    mov         r9,qword ptr [rsp]
    mov         qword ptr [r8+0F8h],r9
    mov         qword ptr [r8+98h],rsp
    mov         r8,qword ptr [rcx+10h]
    mov         qword ptr [rdx+8],r8
    mov         r8,qword ptr [rcx+18h]
    mov         qword ptr [rdx+10h],r8
    mov         r8d,dword ptr [rcx+518h]
    mov         dword ptr [rdx+1748h],r8d
    mov         r8,qword ptr [rcx+510h]
    mov         qword ptr [rdx+17C8h],r8
    mov         r8,qword ptr [rcx+508h]
    mov         qword ptr [rdx+2C8h],r8
    lea         r8,[rcx+30h]
    mov         rbx,qword ptr [r8+90h]
    mov         rbp,qword ptr [r8+0A0h]
    mov         rsi,qword ptr [r8+0A8h]
    mov         rdi,qword ptr [r8+0B0h]
    mov         r12,qword ptr [r8+0D8h]
    mov         r13,qword ptr [r8+0E0h]
    mov         r14,qword ptr [r8+0E8h]
    mov         r15,qword ptr [r8+0F0h]
    movaps      xmm6,xmmword ptr [r8+200h]
    movaps      xmm7,xmmword ptr [r8+210h]
    movaps      xmm8,xmmword ptr [r8+220h]
    movaps      xmm9,xmmword ptr [r8+230h]
    movaps      xmm10,xmmword ptr [r8+240h]
    movaps      xmm11,xmmword ptr [r8+250h]
    movaps      xmm12,xmmword ptr [r8+260h]
    movaps      xmm13,xmmword ptr [r8+270h]
    movaps      xmm14,xmmword ptr [r8+280h]
    movaps      xmm15,xmmword ptr [r8+290h]
    ldmxcsr     dword ptr [r8+34h]
    fldcw       word ptr [r8+100h]
    mov         rsp,qword ptr [r8+98h]
    ret
;
switch_to_fiber endp
_TEXT ends

    end
