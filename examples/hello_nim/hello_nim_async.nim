import std/asyncdispatch
import std/strformat

proc task(id: int): Future[void] {.async.} =
  for loop in 0..2:
    echo &"Hello from task {id}! loops: {loop}"
    if loop < 2:
      await sleepAsync(1000)

proc launch() {.async.} =
  for id in 1..2:
    asyncCheck task(id)
    await sleepAsync(200)
  await task(3)

proc hello_nim() {.exportc, cdecl.} =
  waitFor launch()
  GC_runOrc()
