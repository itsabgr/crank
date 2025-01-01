import {spawn} from "node:child_process";

export interface Result {
    code: number
    stdout: string
    stderr: string
}

export interface Config {
    timeout?: number,
    bin?: string,
    affinity?: number,
    round?: number,
    seed?: string,
}

export default function crank(target: string, config: Config = {}) {

    const args: Array<string> = ['-t', target]

    if (config.seed) {
        args.push('-s', config.seed)
    }

    if (config.affinity) {
        args.push('-a', config.affinity.toString())
    }

    if (config.round) {
        args.push('-r', config.round.toString())
    }


    return new Promise<Result>((resolve, reject) => {

        const bin = config.bin ?? 'crank'

        const child = spawn(bin, args)

        let timer: any;

        let timedout = false

        if (config.timeout) {
            timer = setTimeout(() => {
                if (child.killed) {
                    return
                }
                timedout = true
                child.kill()
            }, config.timeout)
        }

        let stdout = '';
        let stderr = '';

        child.stdout.on('data', (data) => {
            stdout += data.toString();
        });

        child.stderr.on('data', (data) => {
            stderr += data.toString();
        });

        child.once('close', async (code, signal) => {
            child.kill();
            if (config.timeout) {
                clearTimeout(timer);
            }

            if (typeof code == 'number') {
                resolve({
                    code: code as number,
                    stdout,
                    stderr,
                })
            } else if (timedout) {
                reject(new Error('timeout'))
            } else if (signal) {
                reject(new Error(signal))
            } else {
                reject(new Error('killed'))
            }
        })

    })

}

