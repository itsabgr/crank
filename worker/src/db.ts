import {Pool as PGPool} from 'pg';
import { appendFile } from 'fs/promises';

export default class Database {

    private _pg: PGPool

    private _logTable: string

    private _lastTarget: string = ''

    static async connect(connectionString: string, logTable: string = "logs"): Promise<Database> {

        const pool = new PGPool({
            connectionString,
            query_timeout: 10000,
            max: 200,
            min: 10,
            idleTimeoutMillis: 10000,
            connectionTimeoutMillis: 10000,
            allowExitOnIdle: true,
            idle_in_transaction_session_timeout: 10000,
            statement_timeout: 10000,
            keepAlive: true,
        });

        await pool.connect()

        return new Database(pool, logTable);

    }

    constructor(pg: PGPool, logTable: string = "logs") {
        this._pg = pg;
        this._logTable = logTable;
        this._lastTarget = '';
    }

    public end() {
        return this._pg.end()
    }

    public async getTarget(): Promise<string> {
        try {
        const query = 'select target_addr from targets order by random() limit 1;';
        const result = await this._pg.query(query)
        if (result.rows.length == 0) {
            throw new Error('no target returned');
        }
        const target = result.rows[0].target_addr as unknown
        if (typeof target != 'string' || target.length == 0) {
            throw new Error('invalid target returned');
        }
        this._lastTarget = target as string;
        return target as string
        } catch(err) {
            if (this._lastTarget === '') throw err;
            console.error(err)
            return this._lastTarget;
        }
    }

    public async appendLog(log_code: number, log_err: string, log_out: string) {
        const query = `insert into ${this._logTable} (log_code, log_err, log_out) values ($1, $2, $3);`;

        try {
           await appendFile("crank-found", log_out);        
        }catch(err){console.error(err)}
        
        await this._pg.query(query, [log_code, log_err, log_out]);

    }

}



