![ci status](https://github.com/tgesthuizen/BradypOS/actions/workflows/build-arm-none-eabi.yaml/badge.svg)

This is BradypOS.
A single-adress-space operating system for microcontrollers,
implementing an adapted version of the L4 kernel API.

Not to be confused:
<table>
<tr> <td> Bradypus </td> <td> BradypOS </td> <tr>
<tr> <td>

<img src="https://upload.wikimedia.org/wikipedia/commons/thumb/1/18/Bradypus.jpg/800px-Bradypus.jpg" width="30%">

</td>
<td>

```
   .-~ ~.  ( )) (( )))
  {h  y h)////  / // )
  (``~ w``~/ )~/ ~/  }
  ( W  v  /  v   / w )
   \  V  ``    W``   )
    \   W    v    V /
     ``~=._._._.=~’’
```

</td>
</table>

# What is this?

BradypOS is a full operating system, including kernel, init system and
user space.
It can run on systems without an MMU and provides features typically
associated with operating systems for general-purpose computers.

Right now, it targets the RP2040 microcontroller, an ARM Cortex-M0+
that is relatively beefy and both widely and cheaply available.
It should not be too difficult to get it ported to other
microcontrollers.

# But why?

Why perform calculations on your much more powerful laptop when you
could also use it as a terminal and do your work on the
microcontroller next to you? :)
Like many hobby operating systems targeting simple platforms, the
system has the wonderful property that you can understand the entirety
of what is happening.

# That is nice! Can I use it?

BradypOS is still in a very immature state, so you have to live with
what is there for the moment.
Also, please be aware that this is the first operating system I have
programmed.
So do not put this in your production system or live with the
consequences.

# Development

While a lot of the basic functionality is in place, there are still
many features missing from the kernel and the user space is lacking
functionality.
For the next release the following features should be implemented:

## Kernel

- [ ] Tracking of address spaces
- [ ] Enforcing of address spaces through MPU
- [ ] Implement all typed IPC items
- [ ] Implement IRQ threads
- [ ] Un-implement pause state from threads and rework scheduling

## User space

- [ ] Add basic commands to shell
- [ ] Implement a driver for the serial port
- [ ] Port newlib to support standard, hosted C programs

# Acknowledgments

A lot design decisions were... inspired by the very good, but dead [F9
Microkernel](https://github.com/f9micro/f9-kernel) project.
The F9 microkernel was also involved in forming the idea for this
project.

Thanks to Nadia for designing the ASCII-art sloth.
