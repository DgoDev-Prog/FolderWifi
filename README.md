# FolderWifi

Administrador HTTP de la tarjeta SD para Nintendo Switch Homebrew, desarrollado por **Tssr (Diego Ramirez)**. Versión **0.2.0-alpha**.

## Uso

1. Copia `FolderWifi.nro` a `/switch/FolderWifi/` y ejecútalo desde hbmenu.
2. Mantén la consola y el navegador en la misma red local. No hace falta Internet.
3. Abre la dirección que muestra la consola o escanea el QR HTTP con **Y**. El QR vincula el navegador; para abrir la dirección manualmente, introduce el código mostrado en la consola.
4. La aplicación comienza en **solo lectura**. Pulsa **A** en la pantalla principal para habilitar modificaciones. Volver a solo lectura detiene las operaciones modificadoras pendientes; lo ya completado permanece.

| Control | Función |
|---|---|
| A, pantalla principal | Alternar lectura / escritura |
| Y | Ventana QR |
| Izquierda / derecha, ventana QR | HTTP, Wi-Fi, datos Wi-Fi como texto |
| X | Registro de actividad |
| Arriba / abajo, registro | Desplazamiento; A vuelve a seguir los mensajes nuevos |
| ZL | Opciones de conexión y red local |
| A, opciones | Confirmar la opción seleccionada |
| ZL → Dispositivos conectados | Consultar clientes del Wi-Fi creado por FolderWifi |
| Arriba / abajo, dispositivos | Desplazar la lista; B vuelve a opciones de red |
| B | Cerrar la ventana; rechazar la oferta de red local |
| + | Detener servicios y salir |

## Administración web

- Navegación sin recarga, historial del navegador, selección múltiple y por rangos, búsqueda en la carpeta o en subcarpetas, orden y paginación del listado.
- Crear carpetas, copiar y mover árboles completos, renombrar, duplicar, combinar carpetas y consultar propiedades.
- Descargar archivos mediante una autorización temporal; preparar selecciones/carpetas como ZIP; crear ZIP/ZIP64 y extraer ZIP con comprobación de integridad y protección de rutas.
- Papelera con restauración y eliminación definitiva. Las operaciones se ejecutan en segundo plano, con progreso y cancelación.
- Conflictos: reemplazar / combinar, conservar ambos, omitir o cancelar; decisión individual o para los siguientes conflictos. Se revisan conflictos conocidos antes de copiar/mover/renombrar/crear ZIP, y se comprueban nuevamente al ejecutar. Los conflictos adicionales y de extracción se resuelven durante la operación.
- Los archivos que reemplazan a otros se preparan por separado. Un respaldo y un registro de recuperación protegen el cambio de nombre. Al iniciar se revisan reemplazos interrumpidos y se conservan en papelera los respaldos que necesiten revisión.
- Diálogos, avisos y registro propios, temas claro/oscuro, menú contextual y atajos de escritorio. En móviles, selección táctil y barra inferior con todas las acciones; en escritorio el listado tiene desplazamiento independiente.

Cancelar no deshace automáticamente lo terminado. Los resultados diferencian archivos completados, omitidos y reemplazados; los elementos que no se movieron permanecen en el portapapeles.

## Subidas y recuperación

Cada elemento fija su destino al añadirlo. La selección sin confirmar es un **borrador**. «Confirmar lote y subir» convierte ese borrador en un lote; los siguientes lotes confirmados esperan a que termine el activo. Añadir más archivos no confirma el nuevo borrador.

Las subidas utilizan identificadores persistentes, bloques de hasta 1 MiB, offset consultable y CRC32 por bloque y por archivo completo. Una pérdida de respuesta se reconcilia con la Switch antes de repetir datos. Los parciales permanecen en `sdmc:/.folderwifi/uploads/` y se publican al completar y verificar el archivo. CRC32 comprueba corrupción; la autorización es independiente.

La cola y los archivos se conservan en IndexedDB mientras el navegador disponga de espacio y no elimine sus datos. El contenido de cada archivo se guarda una vez; los avances actualizan únicamente el estado. Si la cuota impide conservar archivos, se informa y la pestaña mantiene la selección en memoria. «Guardar pendientes» exporta destinos e identificadores; al recuperarlos hay que volver a seleccionar los originales. Se comprueban tamaño y contenido antes de continuar.

Cerrar/suspender una página detiene su ejecución hasta volver a abrirla. Se consulta entonces el estado remoto. **Cambiar de IP cambia el origen web del navegador**: sus archivos guardados no migran automáticamente. Con la pestaña anterior abierta, «Trasladar a otra IP» abre la dirección local que introduces y comparte la cola y sus archivos con la nueva pestaña; comprueba origen, ventana y un identificador aleatorio del traslado, y pausa la anterior. El borrador permanece sin confirmar. Si esa pestaña está cerrada o el navegador impide abrirla, guarda/recupera los pendientes y vuelve a asociar los originales. Si solo se pierde momentáneamente la conexión y la dirección se conserva, la recuperación es automática.

El selector de carpetas conserva las rutas relativas de los archivos. Para incluir carpetas vacías, arrastra la carpeta al listado desde un navegador de escritorio compatible, o sube un ZIP que las incluya y extráelo. Esto también ofrece una alternativa para móviles sin selector de carpetas.

## Wi-Fi local

Una conexión Wi-Fi o Ethernet que ya tenga IP local se conserva aunque no tenga Internet. Tras perderla se solicitan **cinco intentos** al gestor de red de Nintendo, con hasta 10 segundos por intento y pausas de 2, 4, 8 y 15 segundos. La elección entre perfiles conocidos corresponde al sistema; FolderWifi no enumera ni altera sus redes guardadas.

Si no se recupera la conexión, se ofrece una red local. Al arrancar sin red también se ofrece. «Ahora no» conserva la pantalla y no activa el punto de acceso; **ZL** permite hacerlo más tarde.

El modo local usa **LP2P con WPA2-PSK estándar**, requiere sistema **11.0.0 o posterior** y acceso al servicio por el entorno Homebrew. El servicio proporciona el SSID e IP reales; no se inventa una dirección. Se conserva el perfil devuelto para las siguientes activaciones. La contraseña tiene 20 caracteres aleatorios del servicio criptográfico y permanece hasta cambiarla voluntariamente. Cambiarla requiere reconectar los clientes.

La cabecera identifica **MODO APPLET** o **MODO APLICACIÓN** según el tipo devuelto por libnx; también queda registrado al iniciar. El modo no garantiza que el proceso tenga los identificadores requeridos por LP2P. Tras un rechazo con `00020AE7`, el límite solicitado se redujo de 8 a **1 cliente** para comprobar compatibilidad en hardware. El registro muestra el límite enviado, la operación LP2P que falla y su código; el último error de activación permanece visible aunque exista conexión habitual. Si se cambia la aplicación anfitriona, se vuelve a solicitar el identificador permitido de ese proceso en lugar de reutilizar el del grupo guardado.

En **Y** se muestran tres códigos diferentes:

- **HTTP**: dirección vigente y autorización en el fragmento de la URL, retirado del historial al abrir la página.
- **Wi-Fi**: `WIFI:T:WPA;S:...;P:...;H:false;;`, con escapes y zona libre alrededor del código.
- **Datos Wi-Fi**: nombre, contraseña, seguridad y estado como texto para guardar. Las credenciales también se muestran para introducirlas manualmente.

Después de crear el perfil se pueden consultar sus QR estando la red local inactiva; se indica ese estado. El lector del teléfono decide si reconoce el formato Wi-Fi. **La compilación no confirma compatibilidad del punto de acceso, persistencia del SSID ni lectura de cámaras: se deben probar en la consola y dispositivos reales.**

**ZL → Dispositivos conectados** abre una ventana con contador, IP y MAC de los miembros de la red local. La consulta se actualiza cada segundo desde el trabajador de red. Distingue red inactiva, consulta pendiente, lista vacía y fallo de consulta; una consulta fallida no se presenta como cero clientes. La propia consola se excluye por MAC o IP cuando se puede identificar. No se enumeran los clientes de un router externo ni se deduce el nombre/modelo del dispositivo. La ventana no aumenta el límite configurado de un cliente.

Prueba física comunicada por el desarrollador: creación de la red local, conexión mediante QR Wi-Fi y lectura del QR de credenciales como texto funcionando en el equipo probado. El listado de miembros todavía requiere comprobación tras compilar este cambio.

No se crea una red abierta como alternativa, ni un puente/proxy hacia el teléfono. El servicio utiliza HTTP local, no TLS. Úsalo en redes locales de confianza; la contraseña del Wi-Fi y la vinculación web cumplen funciones distintas.

## Compilación y organización

Requisitos: devkitPro, devkitA64, libnx y portlibs `minizip` / `zlib`. Compilación C++17 mediante `make`; el Makefile produce ELF, NACP y NRO. El icono sigue siendo `icon.jpg`.

| Fuente | Responsabilidad |
|---|---|
| `source/main.cpp` | Inicio/cierre, controles y dibujo; sin transferencias en su bucle |
| `source/gui.cpp/.hpp`, `font8x16.cpp/.h` | Ventanas, QR, registro y framebuffer con su stride real |
| `source/network.cpp`, `runtime.hpp` | Reconexión, perfil WPA2, comandos y estado compartido |
| `source/http_server.cpp` | HTTP, autorización, descargas y trabajador de operaciones |
| `source/upload_manager.cpp/.hpp` | Temporales, offsets, integridad y recuperación de subidas |
| `source/file_manager.cpp/.hpp`, `operations.cpp/.hpp` | Rutas, manipulación, papelera, planificación y búsqueda |
| `source/archive_manager.cpp/.hpp` | ZIP/ZIP64 y extracción |
| `source/notification_manager.cpp/.hpp` | Historial de eventos para la web |
| `source/web_ui.cpp` | HTML, CSS y JavaScript incluidos en el NRO |
| `source/qr_codec.c/.h` | Generador QR de Nayuki, licencia MIT conservada |

El servidor dispone de tres trabajadores HTTP y uno para operaciones de archivos. Todas las respuestas usan envío completo con esperas limitadas. Las modificaciones requieren autorización y POST. Se rechazan parámetros ambiguos, cabeceras duplicadas, cuerpos fuera de límites, rutas fuera de SD, componentes peligrosos, Unicode inválido y acceso web a la configuración interna.

Límites actuales: rutas de hasta 768 bytes UTF-8, componentes de hasta 255 bytes, 64 niveles de recursión, 100.000 elementos por recorrido, 50.000 entradas por carpeta, 5.000 resultados de búsqueda, 512 orígenes seleccionados por operación y 32 trabajos pendientes. El tamaño máximo de un archivo también depende del sistema de archivos de la SD. La fuente de consola representa español/latino básico; otros caracteres usan una sustitución. La web conserva Unicode.

## Verificación

Prueba física pendiente: controles mientras hay transferencias; archivos grandes y falta de espacio; cancelación y conflictos recursivos; pérdida de red y respuesta; reinicio con parciales; entrada/salida de LP2P; cinco reintentos; rechazo de la oferta; contraseña/SSID entre activaciones; los tres QR en Android/iOS. Una operación de copia o ZIP que pierda su seguimiento tras reiniciar el NRO se debe revisar antes de repetirla; la recuperación por identificador persistente corresponde a las subidas.

La versión se mantiene en **0.2.0-alpha**. La prueba de red y QR no acredita todavía estabilidad de todas las operaciones de SD. Antes de etiquetar V1 estable se requieren compilación del estado final y resultados de las pruebas físicas anteriores, incluido el contador al conectar/desconectar un cliente. Los futuros transportes USB/FTP y otras ampliaciones no son requisitos para estabilizar la funcionalidad HTTP actual.

Referencias: [libnx LP2P](https://switchbrew.github.io/libnx/lp2p_8h.html), [ejemplo oficial](https://github.com/switchbrew/switch-examples/tree/master/network/lp2p), [formato Wi-Fi de ZXing](https://github.com/zxing/zxing/wiki/Barcode-Contents#wi-fi-network-config-android-ios-11), [QR-Code-generator de Nayuki](https://github.com/nayuki/QR-Code-generator). Se conserva la licencia MIT en los archivos de la biblioteca QR.
