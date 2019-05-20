#include "LabGPIO.hpp"
#include "utility/log.hpp"

IsrPointer LabGPIO::pin_isr_map[LabGPIO::kPorts][LabGPIO::kPins] = { nullptr };

LabGPIO::LabGPIO(uint8_t port, uint32_t pin)
{
    _port = port;
    _pin = pin;

    switch (port)
    {
        case 0:
            LPC_GPIOx = LPC_GPIO0;
            IOxIntEnR = &LPC_GPIOINT->IO0IntEnR;
            IOxIntEnF = &LPC_GPIOINT->IO0IntEnF;
            break; 
        case 1:
            LPC_GPIOx = LPC_GPIO1;
            break;
        case 2:
            LPC_GPIOx = LPC_GPIO2;
            IOxIntEnR = &LPC_GPIOINT->IO2IntEnR;
            IOxIntEnF = &LPC_GPIOINT->IO2IntEnF;
            break;
        case 3:
            LPC_GPIOx = LPC_GPIO3;
            break;
        case 4:
            LPC_GPIOx = LPC_GPIO4;
            break;
        case 5:
            LPC_GPIOx = LPC_GPIO5;
            break;
        default:
            LOG_INFO("Invalid GPIO port: %d", _port);
            exit(1);
            break;
    }
}

void LabGPIO::Init()
{
    // Enable Pull Down Resistor for SW0 (P1.19) and SW1 (P1.15)
    // CLEAR IOCON, then set to 0x01
    LPC_IOCON->P1_15 &= ~(0x3 << 3);
    LPC_IOCON->P1_15 |= (0x1 << 3);
    // CLEAR IOCON, then set to 0x01
    LPC_IOCON->P1_19 &= ~(0x3 << 3);
    LPC_IOCON->P1_19 |= (0x1 << 3);
}

void LabGPIO::SetAsInput()
{
    LPC_GPIOx->DIR &= ~(1 << _pin);
}

void LabGPIO::SetAsOutput()
{
    LPC_GPIOx->DIR |= (1 << _pin);
}

void LabGPIO::SetDirection(Direction direction)
{
    if(direction == Direction::kOutput)
    {
        SetAsOutput();
    }
    else
    {
        SetAsInput();
    }
}

void LabGPIO::SetHigh()
{
    LPC_GPIOx->PIN |= (1 << _pin); 
}

void LabGPIO::SetLow()
{
    LPC_GPIOx->PIN &= ~(1 << _pin); 
}

void LabGPIO::set(State state)
{
    if(state == State::kHigh)
    {
        SetHigh();
    }
    else
    {
        SetLow();
    }
}

LabGPIO::State LabGPIO::Read()
{
    bool result = ReadBool();
    return static_cast<State>(result);
}

bool LabGPIO::ReadBool()
{
    return (LPC_GPIOx->PIN >> _pin) & 1;
}

void LabGPIO::AttachInterruptHandler(IsrPointer isr, Edge edge)
{
    pin_isr_map[_port][_pin] = isr;
    switch(edge)
    {
        case Edge::kRising:
            *IOxIntEnR |= (1 << _pin);
            *IOxIntEnF &= ~(1 << _pin);
            break;
        case Edge::kFalling:
            *IOxIntEnR &= ~(1 << _pin);
            *IOxIntEnF |= (1 << _pin);
            break;
        case Edge::kBoth:
            *IOxIntEnR |= (1 << _pin);
            *IOxIntEnF |= (1 << _pin);
            break;
        case Edge::kNone:
        default:
            *IOxIntEnR &= ~(1 << _pin);
            *IOxIntEnF &= ~(1 << _pin);
            break;
    }
}

void LabGPIO::EnableInterrupts()
{
    RegisterIsr(GPIO_IRQn, GpioInterruptHandler);
    // NVIC_EnableIRQ(GPIO_IRQn); // Called inside RegisterIsr
}

void LabGPIO::GpioInterruptHandler()
{
    uint32_t IOxIntStatR;
    uint32_t IOxIntStatF;
    volatile uint32_t *IOxIntClr;
    uint8_t port;

    // LOG_INFO("Interrupt Triggered!");
    if(LPC_GPIOINT->IntStatus & 1)                  // Check bit 0 for Port 0 Interrupt Status
    {
        IOxIntStatR = LPC_GPIOINT->IO0IntStatR;
        IOxIntStatF = LPC_GPIOINT->IO0IntStatF;
        IOxIntClr = &LPC_GPIOINT->IO0IntClr;
        port = 0;
    }
    else if((LPC_GPIOINT->IntStatus >> 2) & 1)      // Check bit 2 for Port 2 Interrupt Status
    {
        IOxIntStatR = LPC_GPIOINT->IO2IntStatR;
        IOxIntStatF = LPC_GPIOINT->IO2IntStatF;
        IOxIntClr = &LPC_GPIOINT->IO2IntClr;
        port = 1;
    }

    for(uint8_t i = 0; i < 32; i++)
    {
        if((IOxIntStatR >> i) & 1)
        {
            // LOG_INFO("Rising Edge.");
            pin_isr_map[port][i]();
            *IOxIntClr |= (1 << i);
            break;
        }
        if((IOxIntStatF >> i) & 1)
        {
            // LOG_INFO("Falling Edge.");
            pin_isr_map[port][i]();
            *IOxIntClr |= (1 << i);
            break;
        }
    }
}