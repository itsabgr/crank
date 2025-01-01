import Database from "./db";
import {Config as CrankConfig, Result} from "./crank";
import crank from "./crank";

export default class Runner {

    _db: Database
    _crankConfig: CrankConfig
    stop = false

    constructor(db: Database, crankConfig: Omit<CrankConfig, 'affinity'>) {
        this._db = db;
        this._crankConfig = crankConfig;
        this.stop = false
    }

    async start(affinities: number[]) {
        try {
            const promises: Promise<void>[] = [];

            for (let affinity of affinities) {
                promises.push(this._worker(affinity))
            }

            await Promise.any(promises)
        } finally {
            this.stop = true

        }

    }

    private async _worker(affinity?: number) {

        const config: CrankConfig = {
            timeout: this._crankConfig.timeout,
            bin: this._crankConfig.bin,
            seed: this._crankConfig.seed,
            round: this._crankConfig.round,
            affinity,
        }

        while (!this.stop) {
            const result = await this._do(config)
            if (result.code == 1) {
                throw result;
            }
            await sleep(100)
        }
    }

    private async _do(config: CrankConfig): Promise<Result> {

        const target = await this._db.getTarget()
        const start = process.hrtime.bigint()
        const result = await crank(target, config)
        if (result.code == 0) {
            console.log(result)
            await sleep(100)
        }
        const elapsed = process.hrtime.bigint() - start
        console.log(`${formattedTime(new Date())} ${config.affinity} ${elapsed / 1000000000n}`)
        await this._db.appendLog(result.code, result.stderr, result.stdout);
        return result;

    }

}


export function sleep(ms: number) {
    return new Promise((resolve) => {
        setTimeout(resolve, ms)
    })
}

function formattedTime(now: Date) {


    // Extract components
    const year = now.getFullYear().toString().slice(-2); // Get last two digits
    const month = String(now.getMonth() + 1).padStart(2, '0'); // Months are zero-based
    const day = String(now.getDate()).padStart(2, '0');
    const hours = String(now.getHours()).padStart(2, '0');
    const minutes = String(now.getMinutes()).padStart(2, '0');
    const seconds = String(now.getSeconds()).padStart(2, '0');

    // Combine components
    return `${year}.${month}.${day} ${hours}:${minutes}:${seconds}`
}
