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

En biorreactores de laboratorio de pequeña escala, el control térmico suele realizarse mediante resistencias calefactoras eléctricas y sensores de temperatura integrados al sistema de monitoreo. Sin embargo, obtener mediciones precisas representa un desafío debido a la presencia de ruido electromagnético generado por los sistemas de potencia, como resistencias alimentadas en corriente alterna y fuentes de alimentación conmutadas, lo que puede introducir errores en la lectura del sensor y afectar la estabilidad del sistema de control. 

Sensores de alta precisión como la sonda PT100 ofrecen buena estabilidad y exactitud en la medición de temperatura, pero requieren un adecuado acondicionamiento de señal, referencias de voltaje estables y técnicas de filtrado que permitan minimizar los efectos del ruido eléctrico y las variaciones ambientales (Dos-Reis-Delgado et al., 2023). Si estas consideraciones no se tienen en cuenta, la medición obtenida puede no reflejar la temperatura real del sistema. 

Adicionalmente, mantener la temperatura del biorreactor en un valor de referencia requiere implementar estrategias de control automático capaces de compensar perturbaciones térmicas y dinámicas del sistema. En este contexto, los controladores PID son ampliamente utilizados debido a su simplicidad y efectividad para regular procesos térmicos en aplicaciones industriales y experimentales (Qi et al., 2022). 

Por otra parte, muchos sistemas de laboratorio carecen de herramientas de monitoreo que permitan visualizar en tiempo real el comportamiento del proceso o registrar datos para su análisis posterior. La integración de sistemas de adquisición de datos y monitoreo remoto se ha convertido en una tendencia importante para mejorar la supervisión y el control de procesos en biorreactores modernos (Chastagnier et al., 2026). 

En este contexto, surge la necesidad de desarrollar un sistema embebido capaz de medir, controlar y supervisar de manera robusta la temperatura de un biorreactor de pequeña escala, integrando sensado preciso mediante PT100, filtrado de señal, control PID y herramientas de monitoreo local y remoto que permitan garantizar un control térmico estable y confiable. 

---

## Introducción

El control preciso de variables físicas es un elemento fundamental en múltiples aplicaciones de laboratorio, investigación y procesos industriales. Entre estas variables, la temperatura desempeña un papel crítico en sistemas biológicos y experimentales, donde pequeñas variaciones pueden alterar significativamente los resultados, afectar la estabilidad de los procesos y comprometer la reproducibilidad de los ensayos. 

En el caso de los biorreactores de pequeña escala, utilizados comúnmente en entornos académicos y de investigación, mantener condiciones térmicas estables representa un desafío técnico debido a la presencia de perturbaciones externas, variaciones en la carga térmica y limitaciones propias de los sistemas de bajo costo. La ausencia de un control adecuado puede generar sobrecalentamientos, inestabilidad o tiempos de respuesta ineficientes, como se mencionaba anteriormente. 

Frente a esta necesidad, se propone una solución basada en sistemas embebidos que permita integrar sensado, procesamiento, actuación y supervisión dentro de una arquitectura estructurada y validada. Este proyecto aborda el problema del control térmico en un biorreactor de 2 litros desde una perspectiva de ingeniería, aplicando metodologías formales de diseño, gestión de requisitos y verificación, con el fin de garantizar no solo el funcionamiento del sistema, sino también su confiabilidad, trazabilidad y robustez. 

---

## Alcance del proyecto

Se diseñará e implementará un sistema embebido para el control preciso de temperatura de un biorreactor con capacidad de 2 litros de agua, permitiendo la configuración de un setpoint variable según los requerimientos del proceso. 

- El sistema propuesto contará con las siguientes funcionalidades: 

El sistema será capaz de medir de forma continua la temperatura interna del biorreactor mediante una sonda tipo PT100, garantizando una adquisición confiable de la variable de proceso. 

Para asegurar la calidad de la señal, se implementará filtros analógicos y digitales, con el fin de minimizar la influencia del ruido electromagnético generado por la resistencia calefactora de corriente alterna (AC) y por la fuente de alimentación conmutada. 

El control térmico se realizará mediante la implementación de un algoritmo PID programado en el microcontrolador, permitiendo regular la temperatura con precisión, estabilidad y tiempo de respuesta adecuados. 

La potencia suministrada a la resistencia calefactora será modulada mediante un módulo basado en TRIAC, lo que permitirá controlar eficientemente la energía entregada al sistema térmico. 

- El sistema dispondrá de dos interfaces de usuario: 

Interfaz local: integrada por una pantalla OLED y un conjunto de botones o encoder rotativo, que permitirá configurar el setpoint de temperatura y la rampa de calentamiento directamente desde el equipo. 

Interfaz remota: implementada a través de una aplicación web ejecutable en un PC, la cual recibirá datos de temperatura en tiempo real mediante un protocolo de comunicación (por ejemplo, UART-USB o Bluetooth). Esta aplicación permitirá visualizar gráficamente la evolución temporal de la temperatura y monitorear el estado general del sistema. 

Adicionalmente, se implementará un sistema de gestión de errores asociados al sensor de temperatura, basado en el uso de flags clasificados por niveles de severidad, con el objetivo de mejorar la seguridad y confiabilidad del sistema. 

Finalmente, se incorporarán referencias de voltaje estables que garanticen mediciones robustas frente a variaciones en la temperatura ambiental, asegurando la precisión del sistema de adquisición. 

---

## Objetivo general

Diseñar e implementar un sistema embebido para el control de temperatura de un biorreactor de 2 litros, capaz de mantener un setpoint configurable y un perfil de calentamiento definido mediante un algoritmo PID. El sistema integrará un sensado robusto de temperatura con una sonda PT100, el accionamiento controlado de una resistencia calefactora y dos interfaces de usuario (local y remota) destinadas a la supervisión del proceso y al ajuste de parámetros operativos. 

---

## Objetivos específicos

- Desarrollar el módulo de sensado de temperatura utilizando una sonda PT100, capaz de medir temperaturas en el rango de 10 °C a 65 °C, incorporando acondicionamiento analógico y digital de la señal y referencias de voltaje estables que permitan obtener una lectura de temperatura con un error máximo de ±0,1 °C frente al valor real, incluso en presencia de ruido electromagnético generado por la resistencia calefactora y la fuente de alimentación o ruido por cambios en temperatura ambiental. 

- Diseñar e implementar el algoritmo de control PID en el microcontrolador, de forma que permita mantener la temperatura del biorreactor en un setpoint configurable entre 20 °C y 60 °C, con un error máximo de ±1 °C respecto al valor de referencia, controlando la potencia de la resistencia calefactora mediante un módulo basado en TRIAC accionado desde el microcontrolador y actualizando periódicamente la señal de control para garantizar la estabilidad de la temperatura. 

- Implementar una interfaz de usuario local mediante pantalla OLED y elementos de entrada (botones o encoder) que permita configurar el setpoint de temperatura, la rampa de calentamiento y visualizar el estado básico del sistema. 

- Diseñar e implementar un enlace de comunicación entre el sistema embebido y una aplicación web en un PC (por ejemplo, usando UART–USB o Bluetooth), que permite graficar en tiempo real la evolución de la temperatura, monitorear el estado del sistema y visualizar eventos o errores relevantes. 

- Implementar un esquema de manejo de errores y logging asociado al sensor de temperatura y a los módulos críticos del sistema, utilizando flags y niveles de severidad, de manera que se facilite el diagnóstico durante pruebas y operación. 

---

## Referencias

- Chastagnier, L. et al. (2026). Integration of multi-modal monitoring for dynamic control of large-scale 3D tissue bioreactors. Scientific Reports.

- Dos-Reis-Delgado, A. A. et al. (2022). Recent advances and challenges in temperature monitoring and control in microfluidic devices. Electrophoresis.

- Qi, R. et al. (2022). Design of the PID temperature controller for an alkaline electrolysis system with time delays.
