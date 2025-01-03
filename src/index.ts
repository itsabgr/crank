import Database from "./db";
import Runner from "./runner";
import {z} from "zod";
import {readFileSync} from "fs"
import {cpus} from "os"

const configSchema = z.object({
    bin: z.string().nonempty(),
    timeout: z.number().finite().nonnegative(),
    round: z.number().finite().positive(),
    cpus: z.array(z.number().finite().positive().max(cpus().length - 1)).nonempty().max(cpus().length - 1),
    db: z.string().nonempty(),
});

(async () => {

    const configData = readFileSync(process.argv[process.argv.length - 1])

    const config = configSchema.parse(JSON.parse(configData.toString()))

    console.log(config)

    const db = await Database.connect(config.db)


    const runner = new Runner(db, {
        round: config.round,
        bin: config.bin,
        timeout: config.timeout * 1000,
    })

    try {
        await runner.start(config.cpus)

    } catch (err) {
        console.error(err)
    } finally {
        setTimeout(() => {
            process.exit(0)
        }, 4000)

        await db.end();

    }

})()
