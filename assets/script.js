function colorFromHex(color) {
  return "#" + (color >>> 0).toString(16).padStart(8, "0")
}

window.onload = async () => {
  const app = document.getElementById("app")
  const ctx = app.getContext("2d")

  const wasm = await WebAssembly.instantiateStreaming(fetch("assets/pen.wasm"), {
    env: {
      platformClear: (color) => {
        ctx.fillStyle = colorFromHex(color)
        ctx.fillRect(0, 0, app.width, app.height)
      },

      platformError: (ptr) => {
        const length = new Uint8Array(memory.buffer, ptr).indexOf(0)
        const source = new Uint8Array(memory.buffer, ptr, length)
        console.log("ERROR:", new TextDecoder().decode(source))
      },

      platformDrawLine: (x1, y1, x2, y2, color) => {
        ctx.beginPath()
        ctx.moveTo(x1, y1)
        ctx.lineTo(x2, y2)
        ctx.strokeStyle = colorFromHex(color)
        ctx.stroke()
      }
    }
  })

  const { memory, penUpdate, penRender } = wasm.instance.exports

  const input = await fetch("example").then((f) => f.text())
  const array = new Uint8Array(memory.buffer, 0, input.length)
  array.set(new TextEncoder().encode(input))

  penUpdate(array.byteOffset, array.length)
  penRender()
}
