# Rozšíření GT simulátoru o FreeRTOS-like funkce

## Přehled změn

GT simulátor byl rozšířen o funkce odpovídající základním funkcím FreeRTOS pro správu tasků.

## Nové funkce

| GT funkce | FreeRTOS ekvivalent | Popis |
|-----------|---------------------|-------|
| `gt_suspend(handle)` | `vTaskSuspend()` | Pozastaví task (NULL = aktuální task) |
| `gt_resume(handle)` | `vTaskResume()` | Obnoví pozastavený task |
| `gt_delay(ms)` | `vTaskDelay()` | Uspí aktuální task na daný čas v milisekundách |
| `gt_task_list()` | `vTaskList()` | Vypíše seznam všech tasků a jejich stavů |

## Změny v souborech

### gthr.h

- Přidán typ `gt_handle_t` (pointer na `struct gt_context_t`) pro identifikaci tasků
- Přidána konstanta `MaxTaskName = 16` pro maximální délku jména tasku
- Rozšířena struktura `struct gt_context_t` o:
  - `char name[MaxTaskName]` - jméno tasku
  - `uint64_t delay_until` - čas do kdy je task blokován (v ms)
- Změněna signatura `gt_go()`:
  - Původní: `int gt_go(void (*t_run)(void))`
  - Nová: `int gt_go(void (*t_run)(void), const char *t_name, gt_handle_t *t_handle)`
- Přidány deklarace nových funkcí: `gt_suspend`, `gt_resume`, `gt_delay`, `gt_task_list`

### gthr.c

- Přidána globální proměnná `g_start_time_ms` pro měření času od startu scheduleru
- Přidána pomocná funkce `gt_get_time_ms()` pro získání aktuálního času v milisekundách
- **`gt_init()`**: Inicializuje jméno hlavního vlákna jako "main", nastaví `delay_until = 0`
- **`gt_start_scheduler()`**: Uloží startovní čas scheduleru
- **`gt_sig_handle()`**: Přidána správa timerů - kontroluje všechny blokované tasky a probouzí ty, kterým vypršel `delay_until`
- **`gt_yield()`**: V kooperativním režimu (`GT_PREEMPTIVE=0`) také kontroluje timery
- **`gt_go()`**: 
  - Přidán parametr `t_name` pro pojmenování tasku
  - Přidán parametr `t_handle` pro vrácení handle na vytvořený task
  - Inicializuje `delay_until = 0`
- **Nové funkce**:
  - `gt_suspend()` - pozastaví task, při suspendování aktuálního tasku automaticky volá `gt_yield()`
  - `gt_resume()` - obnoví pozastavený task do stavu Ready
  - `gt_delay()` - nastaví `delay_until` a změní stav na Blocked, pak volá `gt_yield()`
  - `gt_task_list()` - vypíše tabulku tasků s jejich jmény, stavy a ID

### main.c

- Přidán FreeRTOS-like příklad se třemi tasky (Hello, Sleep, Ring) - identické chování jako v `osy-freertos-posix/posix-demo`
- Zachován původní příklad s počítajícími vlákny
- Přidán vlastní testovací příklad pro ověření suspend/resume funkcí
- Program přijímá argument pro výběr příkladu:
  - `./gttest 1` - FreeRTOS příklad (Hello, Sleep, Ring)
  - `./gttest 2` - Původní počítající vlákna
  - `./gttest 3` - Vlastní test suspend/resume

## Stavy tasků

```
Unused    - slot není použit
Running   - aktuálně běžící task
Ready     - připraven k běhu
Blocked   - blokován (čeká na delay)
Suspended - pozastaven (čeká na resume)
```

## Použití

```c
#include "gthr.h"

gt_handle_t my_task_handle;

void my_task(void) {
    while (1) {
        printf("Working...\n");
        gt_delay(1000);  // čekej 1 sekundu
    }
}

void controller_task(void) {
    gt_delay(5000);                    // počkej 5 sekund
    gt_suspend(my_task_handle);        // pozastav my_task
    gt_delay(3000);                    // počkej 3 sekundy
    gt_resume(my_task_handle);         // obnov my_task
}

int main(void) {
    gt_init();
    
    gt_go(my_task, "MyTask", &my_task_handle);
    gt_go(controller_task, "Controller", NULL);
    
    gt_task_list();  // vypíše seznam tasků
    
    gt_start_scheduler();
    return 0;
}
```

## Kompilace

```bash
# Preemptivní režim (výchozí)
make

# Kooperativní režim
make CFLAGS="-g -Wall -DGT_PREEMPTIVE=0"
```

## Porovnání s FreeRTOS

Příklad 1 (`./gttest 1`) se chová identicky jako FreeRTOS příklad v `osy-freertos-posix/posix-demo`:
- Task "Hello" tiskne "Hello world!" každou sekundu
- Task "Ring" čeká 5 sekund a pak probudí task "Sleep"
- Task "Sleep" se sám suspenduje a po probuzení tiskne "Please do not wake up!"
