# Control de Temperatura para Biorreactor mediante Sistema Embebido

## Formulación de Proyecto – Sistemas Embebidos

## Miembros del equipo

- Maria Angelica Díaz Verhelst  
- Sara Hernández Ibarra  
- Sofía Suárez Muñoz  
- Fabián Andrés Vásquez Pino  

---

## Roles del equipo

Cada miembro del equipo asumió un rol con el fin de optimizar la organización del trabajo, distribuir responsabilidades de manera eficiente y garantizar el cumplimiento oportuno de los objetivos del proyecto.

| Integrante | Rol |
|---|---|
| Maria Angelica Díaz Verhelst | Technical Lead |
| Sara Hernández Ibarra | Firmware Engineer |
| Fabián Andrés Vásquez Pino | Hardware Integration Engineer |
| Sofía Suárez Muñoz | Verification & Testing Engineer |

---

## Descripción del problema

En biorreactores de laboratorio de pequeña escala, el control térmico suele realizarse mediante resistencias calefactoras eléctricas y sensores de temperatura integrados al sistema de monitoreo. Sin embargo, obtener mediciones precisas representa un desafío debido a la presencia de **ruido electromagnético generado por los sistemas de potencia**, como resistencias alimentadas en corriente alterna y fuentes de alimentación conmutadas. Este fenómeno puede introducir errores en la lectura del sensor y afectar la estabilidad del sistema de control.

Sensores de alta precisión como la **sonda PT100** ofrecen buena estabilidad y exactitud en la medición de temperatura, pero requieren un adecuado **acondicionamiento de señal**, referencias de voltaje estables y técnicas de filtrado que permitan minimizar los efectos del ruido eléctrico y las variaciones ambientales.

Adicionalmente, mantener la temperatura del biorreactor en un valor de referencia requiere implementar **estrategias de control automático** capaces de compensar perturbaciones térmicas y dinámicas del sistema. En este contexto, los **controladores PID** son ampliamente utilizados debido a su simplicidad y efectividad para regular procesos térmicos.

Por otra parte, muchos sistemas de laboratorio carecen de herramientas de monitoreo que permitan **visualizar en tiempo real el comportamiento del proceso o registrar datos para su análisis posterior**.

En este contexto, surge la necesidad de desarrollar un **sistema embebido capaz de medir, controlar y supervisar de manera robusta la temperatura de un biorreactor de pequeña escala**, integrando sensado preciso mediante PT100, filtrado de señal, control PID y herramientas de monitoreo local y remoto.

---

## Introducción

El control preciso de variables físicas es un elemento fundamental en múltiples aplicaciones de laboratorio, investigación y procesos industriales. Entre estas variables, la **temperatura** desempeña un papel crítico en sistemas biológicos y experimentales, donde pequeñas variaciones pueden alterar significativamente los resultados y comprometer la reproducibilidad de los ensayos.

En el caso de los **biorreactores de pequeña escala**, mantener condiciones térmicas estables representa un desafío técnico debido a la presencia de perturbaciones externas, variaciones en la carga térmica y limitaciones propias de los sistemas de bajo costo.

Este proyecto aborda el problema del **control térmico en un biorreactor de 2 litros**, aplicando metodologías formales de diseño, gestión de requisitos y verificación con el fin de garantizar la confiabilidad, trazabilidad y robustez del sistema.

---

## Alcance del proyecto

Se diseñará e implementará un **sistema embebido para el control preciso de temperatura de un biorreactor con capacidad de 2 litros de agua**, permitiendo la configuración de un **setpoint variable** según los requerimientos del proceso.

### Medición de temperatura

El sistema será capaz de medir de forma continua la temperatura interna del biorreactor mediante una **sonda tipo PT100**, garantizando una adquisición confiable de la variable de proceso.

### Filtrado de señal

Para asegurar la calidad de la señal, se implementarán **filtros analógicos y digitales** con el fin de minimizar la influencia del ruido electromagnético generado por:

- la resistencia calefactora de corriente alterna  
- la fuente de alimentación conmutada  

### Control automático

El control térmico se realizará mediante la implementación de un **algoritmo PID programado en el microcontrolador**, permitiendo regular la temperatura con precisión y estabilidad.

### Control de potencia

La potencia suministrada a la resistencia calefactora será modulada mediante un **módulo basado en TRIAC**, lo que permitirá controlar eficientemente la energía entregada al sistema térmico.

---

## Interfaces de usuario

### Interfaz local

Integrada por:

- pantalla OLED  
- botones o encoder rotativo  

Permitirá:

- configurar el setpoint de temperatura  
- configurar la rampa de calentamiento  
- visualizar el estado del sistema  

### Interfaz remota

Implementada mediante una **aplicación web ejecutable en un PC**, que recibirá datos de temperatura en tiempo real mediante:

- UART–USB  
- Bluetooth  

Permitirá:

- visualizar gráficamente la evolución temporal de la temperatura  
- monitorear el estado del sistema  

---

## Objetivo general

Diseñar e implementar un **sistema embebido para el control de temperatura de un biorreactor de 2 litros**, capaz de mantener un **setpoint configurable y un perfil de calentamiento definido mediante un algoritmo PID**.

---

## Objetivos específicos

### Desarrollo del módulo de sensado

Desarrollar el módulo de sensado utilizando una **sonda PT100**, capaz de medir temperaturas en el rango de **10 °C a 65 °C**, incorporando acondicionamiento analógico y digital de señal.

### Implementación del controlador PID

Diseñar e implementar un **controlador PID en el microcontrolador**, permitiendo mantener la temperatura del biorreactor entre **20 °C y 60 °C** con un error máximo de **±1 °C**.

### Interfaz de usuario local

Implementar una interfaz de usuario mediante **pantalla OLED y botones o encoder** que permita configurar los parámetros del sistema.

### Monitoreo remoto

Implementar comunicación entre el sistema embebido y un **PC**, permitiendo graficar en tiempo real la temperatura y monitorear el estado del sistema.

### Manejo de errores y logging

Implementar un sistema de **manejo de errores y registro de eventos**, utilizando flags y niveles de severidad que faciliten el diagnóstico durante pruebas y operación.

---

## Referencias

Chastagnier, L. et al. (2026). Integration of multi-modal monitoring for dynamic control of large-scale 3D tissue bioreactors. Scientific Reports.

Dos-Reis-Delgado, A. A. et al. (2022). Recent advances and challenges in temperature monitoring and control in microfluidic devices. Electrophoresis.

Qi, R. et al. (2022). Design of the PID temperature controller for an alkaline electrolysis system with time delays.
