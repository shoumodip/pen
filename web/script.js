window.onload = async () => {
  const app = document.getElementById("app")
  const ctx = app.getContext("2d")

  const log = document.getElementById("log")

  const input = document.getElementById("input")
  const style = window.getComputedStyle(input)

  const wasm = await WebAssembly.instantiateStreaming(fetch("web/pen.wasm"), {
    env: {
      platformClear: () => {
        ctx.fillStyle = style.backgroundColor
        ctx.fillRect(0, 0, app.width, app.height)
      },

      platformError: (ptr) => {
        const length = new Uint8Array(memory.buffer, ptr).indexOf(0)
        const source = new Uint8Array(memory.buffer, ptr, length)
        log.value = "Error: " + new TextDecoder().decode(source)
      },

      platformDrawLine: (x1, y1, x2, y2) => {
        ctx.beginPath()
        ctx.moveTo(x1, y1)
        ctx.lineTo(x2, y2)
        ctx.strokeStyle = style.color
        ctx.stroke()
      }
    }
  })

  const { memory, penUpdate, penRender } = wasm.instance.exports

  input.oninput = () => {
    const array = new Uint8Array(memory.buffer, 0, input.value.length)
    array.set(new TextEncoder().encode(input.value))

    log.value = ""
    penUpdate(array.byteOffset, array.length)
    penRender()
  }

  penRender()
  new MutationObserver(penRender).observe(document.querySelector("head"), { childList: true })
}
