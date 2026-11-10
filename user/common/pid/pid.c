#include "pid.h"
#include "../math/math_base.h"
#include "../../config/pidConfig.h"

#include <main.h>

static uint16_t Pid_deltaCalc_method(Pid* self, float sample);

// ---------------Args---------------
extern TIM_HandleTypeDef htim4;

static uint16_t pidSampleCounter[PIDNUM];
static Pid* pidList[PIDNUM];

// ---------------Init---------------
void Pid_hardwareInit(void) {
    // 使用TIM6作为采样周期计时器 周期1ms
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim4);

    // 初始化变量
    for (uint8_t i = 0; i < PIDNUM; i++) {
        pidSampleCounter[i] = 0;
        pidList[i] = NULL;
    }
}

void Pid_hardwareInitFinalize(void) {
    HAL_TIM_Base_Start(&htim4);
}

/**
 * PID初始化
 * @param self 实例
 * @param ts 采样周期 单位ms
 * @param r 设定值
 * @param kp p因子
 * @param ki i因子
 * @param kd d因子
 * @param setMax 输出值上限
 * @param setMin 输出值下限
 * @param tf 微分低通采样时间常数 (ms)
 * @param pidAdjustFunc 参数调整函数指针 传入调整后的值(uint16_t)
 * @param sampleFunc 采样函数指针, 要求该函数要返回float
 * @param getSettingFunc 获得当前参数函数指针, 要求返回uint16_t
 */
void Pid_init(Pid* self, uint16_t ts, float r, float kp, float ki, float kd, uint16_t setMax, uint16_t setMin,
              float tf, pidAdjustFunc pidAdjustFunc, sampleFunc sampleFunc,
              getSettingFunc getSettingFunc) {
    // 采样相关
    self->ts = ts;

    // pid运算相关
    self->r = r;
    self->kp = kp;
    self->ki = ki;
    self->kd = kd;
    self->setMax = setMax;
    self->setMin = setMin;

    self->dAlpha = tf / ((float)ts + tf);

    self->pidAdjustFunc = pidAdjustFunc;
    self->sampleFunc = sampleFunc;
    self->getSettingFunc = getSettingFunc;
}

// ---------------Method---------------
/**
 * PID运算
 * @param sample 采样值
 * @return 设定值
 */
static uint16_t Pid_deltaCalc_method(Pid* self, float sample) {
    float eTemp = self->r - sample;

    // P
    self->uAcc += self->kp * (eTemp - self->eBefore);

    // I
    self->uAcc += self->ki * (self->eBefore + eTemp) * (float)self->ts * 0.5f * 0.001f;
    self->eBefore = eTemp;

    // D
    float dy = sample - self->yBefore;
    self->yBefore = sample;
    float dRaw = -self->kd * dy / ((float)self->ts * 0.001f);  // 微分先行
    float dNew = self->dAlpha * self->dState + (1.0f - self->dAlpha) * dRaw;  // 低通滤波
    self->uAcc += (1.0f - self->dAlpha) * (dRaw - self->dState);
    self->dState = dNew;

    // 设定值限幅
    if (self->uAcc > (float)self->setMax) {
        self->uAcc = (float)self->setMax;
    }
    else if (self->uAcc < (float)self->setMin) {
        self->uAcc = (float)self->setMin;
    }

    return (uint16_t)self->uAcc;
}

/**
 * 启动PID闭环控制
 */
void Pid_start_method(Pid* self) {
    self->fristRun = 1;
    self->uAcc = self->getSettingFunc();

    // pid实例管理
    for (uint8_t i = 0; i < PIDNUM; i++) {
        if (pidList[i] == NULL) {
            pidList[i] = self;
            break;
        }
    }
}

/**
 * 停止PID闭环控制
 */
void Pid_stop_method(Pid* self) {
    for (uint8_t i = 0; i < PIDNUM; i++) {
        if (pidList[i] == self) {
            pidList[i] = NULL;
            break;
        }
    }

    self->yBefore = 0.0f;
    self->eBefore = 0.0f;
}

// ---------------Setter---------------
void Pid_r_setter(Pid* self, uint16_t r) {
    self->r = r;
}

// ---------------interruptCallFunction---------------
void Pid_TIM4_interruptCallFunction(void) {
    for (uint8_t i = 0; i < PIDNUM; i++) {
        if (pidList[i] == NULL || pidList[i]->ts == 0) { continue; }

        if (pidList[i]->fristRun) {
            pidList[i]->fristRun = 0;
            float temp = pidList[i]->sampleFunc();
            pidList[i]->yBefore = temp;
            pidList[i]->eBefore = 0.0f;
            pidList[i]->dState = 0.0f;
        }

        pidSampleCounter[i]++;
        if (pidList[i]->ts == pidSampleCounter[i]) {
            pidSampleCounter[i] = 0;
            float temp = pidList[i]->sampleFunc();
            pidList[i]->pidAdjustFunc(Pid_deltaCalc_method(pidList[i], temp));
        }
    }
}
