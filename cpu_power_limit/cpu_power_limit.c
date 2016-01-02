//
//  cpu_power_limit.c
//  cpu_power_limit
//
//  Created by MhkErriy on 15/12/29.
//  Copyright © 2015年 Erriy. All rights reserved.
//

#include <mach/mach_types.h>


#define ___MESSAGE__

#ifdef ___MESSAGE__
    #define MESSAGE_OUT(format, args...)                            \
        printf("cpu_power_limit:\t"format"\n",##args)
#else
    #define MESSAGE_OUT(format, args...)
#endif


#define IA32_MISC_ENABLE                    0x1a0
#define MSR_RAPL_POWER_UNIT                 0x606
#define MSR_PKG_POWER_LIMIT                 0x610


#ifndef u32
    #define u32                             unsigned int
#endif


union pkg_power
{
    long long       limit64;
    
    struct
    {
        u32         low;
        u32         high;
    }limit32;

    struct
    {
        u32         power_limit1        :15;
        u32         enable1             :1;
        u32         clamping1           :1;
        u32         time_limit1         :7;
        u32         :8;
        u32         power_limit2        :15;
        u32         enable2             :1;
        u32         clamping2           :1;
        u32         time_limit2         :7;
        u32         :7;
        u32         lock_until_reset    :1;
    }limit;

};


union pkg_power system_backup   = {0};





kern_return_t cpu_power_limit_start(kmod_info_t * ki, void *d);
kern_return_t cpu_power_limit_stop(kmod_info_t *ki, void *d);




int
__pow(int a, int b)
{
    int ret = 1;
    while(b--)
        ret*=a;
    return ret;
}


void
turbo_boost_turn_off(void)
{
    __asm{
        mov ecx, IA32_MISC_ENABLE
        rdmsr
        or edx, 40h
        wrmsr
    }
}


void
turbo_boost_turn_on(void)
{
    __asm{
        mov ecx, IA32_MISC_ENABLE
        rdmsr
        and edx, 0ffffffbfh
        wrmsr
    }
}


void
print_turbo_boost_state(void)
{
#define TURBO_BOOST_MASK    0b10
    
    u32 result = 0;
    
    __asm {
        mov eax, 06h
        cpuid
        mov result, eax
    }
    
    if(TURBO_BOOST_MASK & result)
    {
        MESSAGE_OUT("\t[+]turbo boost is on!");
    }
    else
    {
        MESSAGE_OUT("\t[-]turbo boost is off!");
    }
    
#undef TURBO_BOOST_MASK
}


void
__do_set_power_limit(union pkg_power *pp)
{
    u32     a   = pp->limit32.low;
    u32     d   = pp->limit32.high;
    __asm{
        mov ecx, MSR_PKG_POWER_LIMIT
        mov eax, a
        mov edx, d
        wrmsr
    }
}


long long
__do_get_power_limit(void)
{
    union pkg_power result  = {0};

    u32     a   = 0;
    u32     d   = 0;

    __asm{
        mov ecx, MSR_PKG_POWER_LIMIT
        rdmsr
        mov a, eax
        mov d, edx
    }

    result.limit32.low  = a;
    result.limit32.high = d;

    return result.limit64;
}


u32
__do_get_power_unit(void)
{
    u32    result  = 0;
    __asm{
        mov ecx, MSR_RAPL_POWER_UNIT
        rdmsr
        mov result, eax
    }

    return result & 0xf;
}


int
set_my_power_limit(u32 pl1, u32 pl2, u32 lock)
{
    union pkg_power pp          = {0};
    u32             power_unit  = 0;

    pp.limit64                  = __do_get_power_limit();
    system_backup.limit64       = pp.limit64;

    if(pp.limit.lock_until_reset)
    {
        MESSAGE_OUT("\t[!]power limit lock is on!");
        return -1;
    }


    power_unit  = __do_get_power_unit();
    power_unit  = __pow(2, power_unit);

    pp.limit.power_limit1       = pl1 * power_unit;
    pp.limit.power_limit2       = pl2 * power_unit;

    if( pp.limit.power_limit1  != pl1 * power_unit ||
        pp.limit.power_limit2  != pl2 * power_unit )
    {
        MESSAGE_OUT("\t[!]power_limit overflow!");
        return -1;
    }


    pp.limit.enable1            = 1;
    pp.limit.enable2            = 1;
    pp.limit.clamping1          = 1;
    pp.limit.clamping2          = 1;
    pp.limit.lock_until_reset   = lock ? 1 : 0;

    __do_set_power_limit(&pp);

    MESSAGE_OUT("\t[.]power_limit #1 : %d w", pl1);
    MESSAGE_OUT("\t[.]power_limit #2 : %d w", pl2);

    return 0;
}


int
resume_system_power_limit(void)
{
    if(!system_backup.limit64)
    {
        MESSAGE_OUT("\t[!]system_power_limit unknown!");
        return -1;
    }
    __do_set_power_limit(&system_backup);

    if(__do_get_power_limit() != system_backup.limit64)
    {
        MESSAGE_OUT("\t[!]power_limit may locked! cannot resume to %p", \
                        system_backup.limit64);
        return -1;
    }

    MESSAGE_OUT("\t[.]power_limit resume to system");
    return 0;
}



kern_return_t
cpu_power_limit_start(kmod_info_t * ki, void *d)
{
    MESSAGE_OUT("[+]===================>%s in\n", __FUNCTION__);

    set_my_power_limit(15,15,0);
    turbo_boost_turn_off();
    print_turbo_boost_state();

    MESSAGE_OUT("[+]===================>%s out\n", __FUNCTION__);
    
    return KERN_SUCCESS;
}

kern_return_t
cpu_power_limit_stop(kmod_info_t *ki, void *d)
{
    MESSAGE_OUT("[-]===================>%s in\n", __FUNCTION__);

    turbo_boost_turn_on();
    print_turbo_boost_state();
    resume_system_power_limit();

    MESSAGE_OUT("[-]===================>%s out\n", __FUNCTION__);

    return KERN_SUCCESS;
}





