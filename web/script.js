window.onload = async () => {
  const app = document.getElementById("app")
  const ctx = app.getContext("2d")

  const run = document.getElementById("run")
  const input = document.getElementById("input")
  const style = window.getComputedStyle(input)

  const error = document.getElementById("error")
  const errorSpace = Number(style.fontSize.slice(0, -2)) * 3.3

  const wasm = await WebAssembly.instantiateStreaming(fetch("web/pen.wasm"), {
    env: {
      platformClear: () => {
        ctx.fillStyle = style.backgroundColor
        ctx.fillRect(0, 0, app.width, app.height)
      },

      platformErrorStart: () => {
        error.style.backgroundColor = "#FF000066"
        error.value = "Error: "
      },

      platformErrorPush: (start, count) => {
        error.value += new TextDecoder().decode(new Uint8Array(memory.buffer, start, count))
      },

      platformErrorEnd: () => { },

      platformDrawLine: (x1, y1, x2, y2) => {
        ctx.beginPath()
        ctx.moveTo(x1, y1)
        ctx.lineTo(x2, y2)
        ctx.strokeStyle = style.color
        ctx.stroke()
      }
    }
  })

  const { memory, penInit, penRender, penUpdate } = wasm.instance.exports

  penInit()
  run.onclick = () => {
    const array = new Uint8Array(memory.buffer, 0, input.value.length)
    array.set(new TextEncoder().encode(input.value))

    error.value = ""
    error.style.backgroundColor = "#00FF0066"

    penUpdate(array.byteOffset, array.length)
    penRender(app.width, app.height)
  }

  input.value = await fetch("example").then((e) => e.text())
  run.onclick()

  window.onresize = () => {
    app.width = window.innerWidth * 0.6
    app.height = window.innerHeight - errorSpace
    penRender(app.width, app.height)
  }

  window.onresize()

  new MutationObserver(() => penRender(app.width, app.height))
    .observe(document.querySelector("head"), { childList: true })
}
