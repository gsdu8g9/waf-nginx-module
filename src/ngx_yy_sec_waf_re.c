/*
** @file: ngx_yy_sec_waf_re.c
** @description: This is the rule rule engine for yy sec waf.
** @author: dw_liqi1<liqi1@yy.com>
** @date: 2013.07.15
** Copyright (C) YY, Inc.
*/

#include "ngx_yy_sec_waf.h"

static yy_sec_waf_re_t *rule_engine;

/*
** @description: This function is called to resolve variables in hash.
** @para: ngx_str_t *variable
** @return: static re_var_metadata *
*/

static re_var_metadata *
yy_sec_waf_re_resolve_variable_in_hash(ngx_str_t *variable)
{
    ngx_uint_t key;
    re_var_metadata *metadata;

    if (variable == NULL) {
        return NULL;
    }

    key = ngx_hash_key_lc(variable->data, variable->len);
    if (variable->data[0] != '$') {
        ngx_strlow(variable->data, variable->data, variable->len);
    }

    metadata = (re_var_metadata *)ngx_hash_find(
        &rule_engine->variables_in_hash, key, variable->data, variable->len);

    return metadata;
}

/*
** @description: This function is called to resolve tfns in hash.
** @para: ngx_str_t *action
** @return: static re_action_metadata *
*/

re_tfns_metadata *
yy_sec_waf_re_resolve_tfn_in_hash(ngx_str_t *tfn)
{
    ngx_uint_t key;
    re_tfns_metadata *metadata;

    if (tfn == NULL) {
        return NULL;
    }

    key = ngx_hash_key_lc(tfn->data, tfn->len);
    ngx_strlow(tfn->data, tfn->data, tfn->len);

    metadata = (re_tfns_metadata *)ngx_hash_find(
        &rule_engine->tfns_in_hash, key, tfn->data, tfn->len);

    return metadata;
}

/*
** @description: This function is called to resolve operators in hash.
** @para: ngx_str_t *operator
** @return: static re_op_metadata *
*/

static re_op_metadata *
yy_sec_waf_re_resolve_operator_in_hash(ngx_str_t *operator)
{
    ngx_uint_t key;
    re_op_metadata *metadata;

    if (operator == NULL) {
        return NULL;
    }

    key = ngx_hash_key_lc(operator->data, operator->len);
    ngx_strlow(operator->data, operator->data, operator->len);

    metadata = (re_op_metadata *)ngx_hash_find(
        &rule_engine->operators_in_hash, key, operator->data, operator->len);

    return metadata;
}

/*
** @description: This function is called to redirect request url to the denied url of yy sec waf.
** @para: ngx_http_request_t *r
** @para: ngx_http_request_ctx_t *ctx
** @return: NGX_HTTP_OK or NGX_ERROR if failed.
*/

static ngx_int_t
yy_sec_waf_output_forbidden_page(ngx_http_request_t *r,
    ngx_http_request_ctx_t *ctx)
{
    ngx_http_yy_sec_waf_loc_conf_t *cf;
    ngx_str_t  empty = ngx_string("");
    ngx_str_t *tmp_uri;

    cf = ngx_http_get_module_loc_conf(r, ngx_http_yy_sec_waf_module);

    if (cf->denied_url) {
        tmp_uri = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
        if (!tmp_uri)
            return NGX_ERROR;
        
        tmp_uri->len = r->uri.len + (2 * ngx_escape_uri(NULL, r->uri.data, r->uri.len,
            NGX_ESCAPE_ARGS));
        tmp_uri->data = ngx_pcalloc(r->pool, tmp_uri->len+1);
    
        ngx_escape_uri(tmp_uri->data, r->uri.data, r->uri.len, NGX_ESCAPE_ARGS);
        
        ngx_table_elt_t *h;
        
        if (r->headers_in.headers.last)	{
            h = ngx_list_push(&(r->headers_in.headers));
            h->key.len = ngx_strlen("orig_url");
            h->key.data = ngx_pcalloc(r->pool, ngx_strlen("orig_url")+1);
            ngx_memcpy(h->key.data, "orig_url", ngx_strlen("orig_url"));
            h->lowcase_key = ngx_pcalloc(r->pool, ngx_strlen("orig_url") + 1);
            ngx_memcpy(h->lowcase_key, "orig_url", ngx_strlen("orig_url"));
            h->value.len = tmp_uri->len;
            h->value.data = ngx_pcalloc(r->pool, tmp_uri->len+1);
            ngx_memcpy(h->value.data, tmp_uri->data, tmp_uri->len);
            
            h = ngx_list_push(&(r->headers_in.headers));
            h->key.len = ngx_strlen("orig_args");
            h->key.data = ngx_pcalloc(r->pool, ngx_strlen("orig_args")+1);
            ngx_memcpy(h->key.data, "orig_args", ngx_strlen("orig_args"));
            h->lowcase_key = ngx_pcalloc(r->pool, ngx_strlen("orig_args") + 1);
            ngx_memcpy(h->lowcase_key, "orig_args", ngx_strlen("orig_args"));
            h->value.len = r->args.len;
            h->value.data = ngx_pcalloc(r->pool, r->args.len+1);
            ngx_memcpy(h->value.data, r->args.data, r->args.len);
            
            h = ngx_list_push(&(r->headers_in.headers));
            h->key.len = ngx_strlen("yy_sec_waf");
            h->key.data = ngx_pcalloc(r->pool, ngx_strlen("yy_sec_waf")+1);
            ngx_memcpy(h->key.data, "yy_sec_waf", ngx_strlen("yy_sec_waf"));
            h->lowcase_key = ngx_pcalloc(r->pool, ngx_strlen("yy_sec_waf") + 1);
            ngx_memcpy(h->lowcase_key, "yy_sec_waf", ngx_strlen("yy_sec_waf"));
            h->value.len = empty.len;
            h->value.data = empty.data;
        }

        ngx_http_internal_redirect(r, cf->denied_url, &empty);

        return NGX_HTTP_OK;
    } else {
        return NGX_HTTP_PRECONDITION_FAILED;
    }
}

/*
** @description: This function is called to resolve actions in hash.
** @para: ngx_str_t *action
** @return: static re_action_metadata *
*/

re_action_metadata *
yy_sec_waf_re_resolve_action_in_hash(ngx_str_t *action)
{
    ngx_uint_t key;
    re_action_metadata *metadata;

    if (action == NULL) {
        return NULL;
    }

    key = ngx_hash_key_lc(action->data, action->len);
    ngx_strlow(action->data, action->data, action->len);

    metadata = (re_action_metadata *)ngx_hash_find(
        &rule_engine->actions_in_hash, key, action->data, action->len);

    return metadata;
}

/*
** @description: This function is called to execute operator.
** @para: ngx_http_request_t *r
** @para: ngx_str_t *str
** @para: ngx_http_yy_sec_waf_rule_t *rule
** @para: ngx_http_request_ctx_t *ctx
** @return: RULE_MATCH or RULE_NO_MATCH if failed.
*/

static ngx_int_t
yy_sec_waf_re_op_execute(ngx_http_request_t *r,
    ngx_str_t *str, ngx_http_yy_sec_waf_rule_t *rule, ngx_http_request_ctx_t *ctx)
{
    ngx_int_t rc;

    rc = rule->op_metadata->execute(r, str, rule);

    if ((rc == RULE_MATCH && !rule->op_negative)
        || (rc == RULE_NO_MATCH && rule->op_negative)) {
        ctx->matched = 1;
        ctx->rule_id = rule->rule_id;
        ctx->allow = rule->allow;
        ctx->block = rule->block;
        ctx->log = rule->log;
        ctx->gids = rule->gids;
        ctx->msg = rule->msg;
        ctx->matched_string = str;
        return RULE_MATCH;
    }

    return RULE_NO_MATCH;
}

/*
** @description: This function is called to process normal rules for yy sec waf.
** @para: ngx_http_request_t *r
** @para: ngx_http_yy_sec_waf_loc_conf_t *cf
** @para: ngx_http_request_ctx_t *ctx
** @return: RULE_MATCH or RULE_NO_MATCH if failed.
*/

ngx_int_t
yy_sec_waf_re_process_normal_rules(ngx_http_request_t *r,
    ngx_http_yy_sec_waf_loc_conf_t *cf, ngx_http_request_ctx_t *ctx, ngx_uint_t phase)
{
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "[ysec_waf] yy_sec_waf_re_process_normal_rules Entry");

    ngx_uint_t                  i, rule_num;
    ngx_int_t                   rc;
    ngx_str_t                   var;
    ngx_http_yy_sec_waf_rule_t *rule;
    ngx_http_variable_value_t  *vv;

    rule = NULL;
    rule_num = 0;

    if (phase == REQUEST_HEADER_PHASE) {
        if (ctx->cf->request_header_rules == NULL) {
            return NGX_ERROR;
        }

        rule = cf->request_header_rules->elts;
        rule_num = cf->request_header_rules->nelts;
    } else if (phase == REQUEST_BODY_PHASE) {
        if (ctx->cf->request_body_rules == NULL) {
            return NGX_ERROR;
        }

        rule = cf->request_body_rules->elts;
        rule_num = cf->request_body_rules->nelts;
    } else if (phase == RESPONSE_HEADER_PHASE) {
        if (ctx->cf->request_body_rules == NULL) {
            return NGX_ERROR;
        }

        rule = cf->response_header_rules->elts;
        rule_num = cf->response_header_rules->nelts;
    } else if (phase == RESPONSE_BODY_PHASE) {
        if (ctx->cf->request_body_rules == NULL) {
            return NGX_ERROR;
        }

        rule = cf->response_body_rules->elts;
        rule_num = cf->response_body_rules->nelts;
    } 

    ctx->phase = phase;

    for (i=0; i < rule_num; i++) {

        if (rule[i].var_metadata == NULL || rule[i].var_metadata->generate == NULL)
            continue;

        vv = ngx_palloc(r->pool, sizeof(ngx_http_variable_value_t));

        rc = rule[i].var_metadata->generate(&rule[i], ctx, vv);

        if (rc == NGX_ERROR || vv->not_found) {
            continue;
        }

        if (rule[i].tfn_metadata != NULL) {
            rc = rule[i].tfn_metadata->execute(vv);
            if (rc == NGX_ERROR) {
                ngx_log_error(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "[ysec_waf] failed to execute tfns");
                return NGX_ERROR;
            }
        }

        var.data = vv->data;
        var.len = vv->len;

        rc = yy_sec_waf_re_op_execute(r, &var, &rule[i], ctx);

        if (rc == NGX_ERROR) {
            return rc;
        } else if (rc == RULE_MATCH) {
            goto MATCH;
        } else if (rc == RULE_NO_MATCH) {
            continue;
        }
    }

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "[ysec_waf] yy_sec_waf_re_process_normal_rules Exit");

    return NGX_DECLINED;

MATCH:
    ngx_atomic_fetch_add(request_matched, 1);

    ctx->process_done = 1;

    if (ctx->log)
        ngx_atomic_fetch_add(request_logged, 1);

    if (ctx->allow)
        ngx_atomic_fetch_add(request_allowed, 1);
    
    if (ctx->block)
        ngx_atomic_fetch_add(request_blocked, 1);

    if (ctx->log && ctx->matched_string) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
            "[ysec_waf] %s, id:%d, conn_per_ip:%ud,"
            " matched:%uA, blocked:%uA, allowed:%uA, alerted:%uA"
            " msg:%V, info:%V",
            ctx->block? "block": ctx->allow? "allow": "alert",
            ctx->rule_id, ctx->conn_per_ip,
            *request_matched, *request_blocked, *request_allowed, *request_logged,
            ctx->msg, ctx->process_body_error? &ctx->process_body_error_msg:ctx->matched_string);
    }

    if (ctx->allow)
        return NGX_DECLINED;

    return yy_sec_waf_output_forbidden_page(r, ctx);
}

/*
** @description: This function is called to process the request.
** @para: ngx_http_request_t *r
** @para: ngx_conf_t *cf
** @para: ngx_http_request_ctx_t *ctx
** @return: NGX_OK or NGX_ERROR if failed.
*/

ngx_int_t
ngx_http_yy_sec_waf_process_request(ngx_http_request_t *r,
    ngx_http_yy_sec_waf_loc_conf_t *cf, ngx_http_request_ctx_t *ctx)
{
    ngx_int_t rc;

    if ((r->method == NGX_HTTP_POST || r->method == NGX_HTTP_PUT)
        && r->request_body) {
        ngx_http_yy_sec_waf_process_body(r, cf, ctx);

        rc = yy_sec_waf_re_process_normal_rules(r, cf, ctx, REQUEST_BODY_PHASE);
        if (ctx->matched || rc == NGX_ERROR) {
            return rc;
        }
    }

    return NGX_DECLINED;
}

/*
** @description: This function is called to read configuration of yy sec waf.
** @para: ngx_conf_t *cf
** @para: ngx_command_t *cmd
** @para: void *conf
** @return: NGX_CONF_OK or NGX_CONF_ERROR if failed.
*/

char *
ngx_http_yy_sec_waf_re_read_conf(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf)
{
    ngx_http_yy_sec_waf_loc_conf_t  *p = conf;

    ngx_uint_t        n;
    u_char           *pos;
    ngx_str_t        *value, variable, operator, action;
    ngx_http_yy_sec_waf_rule_t rule, *rule_p;
    ngx_shm_zone_t *shm_zone;

    value = cf->args->elts;
    ngx_memset(&rule, 0, sizeof(ngx_http_yy_sec_waf_rule_t));

    /* variable */
    if (value[1].data[0] == '$') {
        variable.data = &value[1].data[1];
        variable.len = value[1].len-1;
        rule.var_index = ngx_http_get_variable_index(cf, &variable);
        ngx_str_set(&variable, "$");
    } else {
        ngx_memcpy(&variable, &value[1], sizeof(ngx_str_t));
    }

    rule.var_metadata = yy_sec_waf_re_resolve_variable_in_hash(&variable); 

    if (rule.var_metadata == NULL) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "[ysec_waf] Failed to resolve variable");
        return NGX_CONF_ERROR;
    }

    /* operator */
    ngx_memcpy(&operator, &value[2], sizeof(ngx_str_t));

    if (operator.data[0] == '!') {
        rule.op_negative = 1;
        operator.data++;
    }

    pos = ngx_strlchr(operator.data, operator.data+operator.len, ':');
    operator.len = pos-operator.data;

    rule.op_metadata = yy_sec_waf_re_resolve_operator_in_hash(&operator);

    if (rule.op_metadata == NULL) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "[ysec_waf] Failed to resolve operator");
        return NGX_CONF_ERROR;
    }

    operator.len = value[2].len;
    if (operator.data[0] == '!') {
        operator.len--;
    }

    if (rule.op_metadata->parse(cf, &operator, &rule) != NGX_CONF_OK) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "[ysec_waf] Failed parsing '%V'", &operator);
        return NGX_CONF_ERROR;
    }

    /* action */
    for (n = 3; n < cf->args->nelts; n++) {
        ngx_memcpy(&action, &value[n], sizeof(ngx_str_t));
        u_char *pos = ngx_strlchr(action.data, action.data+action.len, ':');
        action.len = pos-action.data;

        rule.action_metadata = yy_sec_waf_re_resolve_action_in_hash(&action);

        if (rule.action_metadata == NULL) {
            ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "[ysec_waf] Failed to resolve action");
            return NGX_CONF_ERROR;
        }

        if (rule.action_metadata->parse(cf, &value[n], &rule) != NGX_CONF_OK) {
            ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "[ysec_waf] Failed parsing '%V'", &action);
            return NGX_CONF_ERROR;
        }
    }

    if (rule.phase & REQUEST_HEADER_PHASE) {
        if (p->request_header_rules == NULL) {
            p->request_header_rules = ngx_array_create(cf->pool, 1, sizeof(ngx_http_yy_sec_waf_rule_t));

            if (p->request_header_rules == NULL)
                return NGX_CONF_ERROR;
        }

        rule_p = ngx_array_push(p->request_header_rules);

        if (rule_p == NULL)
            return NGX_CONF_ERROR;

        ngx_memcpy(rule_p, &rule, sizeof(ngx_http_yy_sec_waf_rule_t));
    }

    if (rule.phase & REQUEST_BODY_PHASE) {
        if (p->request_body_rules == NULL) {
            p->request_body_rules = ngx_array_create(cf->pool, 1, sizeof(ngx_http_yy_sec_waf_rule_t));

            if (p->request_body_rules == NULL)
                return NGX_CONF_ERROR;
        }

        rule_p = ngx_array_push(p->request_body_rules);

        if (rule_p == NULL)
            return NGX_CONF_ERROR;

        ngx_memcpy(rule_p, &rule, sizeof(ngx_http_yy_sec_waf_rule_t));
    }

    if (rule.phase & RESPONSE_HEADER_PHASE) {
        if (p->response_header_rules == NULL) {
            p->response_header_rules = ngx_array_create(cf->pool, 1, sizeof(ngx_http_yy_sec_waf_rule_t));

            if (p->response_header_rules == NULL)
                return NGX_CONF_ERROR;
        }

        rule_p = ngx_array_push(p->response_header_rules);

        if (rule_p == NULL)
            return NGX_CONF_ERROR;

        ngx_memcpy(rule_p, &rule, sizeof(ngx_http_yy_sec_waf_rule_t));
    }

    if (rule.phase & RESPONSE_BODY_PHASE) {
        if (p->response_body_rules == NULL) {
            p->response_body_rules = ngx_array_create(cf->pool, 1, sizeof(ngx_http_yy_sec_waf_rule_t));

            if (p->response_body_rules == NULL)
                return NGX_CONF_ERROR;
        }

        rule_p = ngx_array_push(p->response_body_rules);

        if (rule_p == NULL)
            return NGX_CONF_ERROR;

        ngx_memcpy(rule_p, &rule, sizeof(ngx_http_yy_sec_waf_rule_t));
    }

    // Create shm zone for conn processor.
    if (p->conn_processor) {
        shm_zone = ngx_http_yy_sec_waf_create_shm_zone(cf);

        if (shm_zone != NULL) {
            p->shm_zone = shm_zone;
        }
    }

    return NGX_CONF_OK;
}

/*
** @description: This function is called to read denied url of yy sec waf.
** @para: ngx_conf_t *cf
** @para: ngx_str_t *tmp
** @para: ngx_http_yy_sec_waf_rule_t *rule
** @return: NGX_CONF_OK or NGX_CONF_ERROR if failed.
*/

char *
ngx_http_yy_sec_waf_re_read_du_loc_conf(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf)
{
    ngx_http_yy_sec_waf_loc_conf_t *p = conf;
    ngx_str_t *value;

    value = cf->args->elts;
    if (value[1].len == 0)
        return NGX_CONF_ERROR;

    p->denied_url = ngx_pcalloc(cf->pool, sizeof(ngx_str_t));
    if (!p->denied_url)
        return NGX_CONF_ERROR;

    p->denied_url->data = ngx_pcalloc(cf->pool, value[1].len+1);
    if (!p->denied_url->data)
        return NGX_CONF_ERROR;

    ngx_memcpy(p->denied_url->data, value[1].data, value[1].len);
    p->denied_url->len = value[1].len;

    return NGX_CONF_OK;
}

/*
** @description: This function is called to create rule engine for yy sec waf.
** @para: ngx_conf_t *cf
** @return: NGX_OK or NGX_ERROR if failed.
*/

ngx_int_t
ngx_http_yy_sec_waf_re_create(ngx_conf_t *cf)
{
    rule_engine = ngx_pcalloc(cf->pool, sizeof(yy_sec_waf_re_t));
    if (rule_engine == NULL) {
        return NGX_ERROR;
    }

    if (ngx_http_yy_sec_waf_init_variables_in_hash(cf,
            &rule_engine->variables_in_hash) == NGX_ERROR)
        return NGX_ERROR;

    if (ngx_http_yy_sec_waf_init_operators_in_hash(cf,
            &rule_engine->operators_in_hash) == NGX_ERROR)
        return NGX_ERROR;

    if (ngx_http_yy_sec_waf_init_actions_in_hash(cf,
            &rule_engine->actions_in_hash) == NGX_ERROR)
        return NGX_ERROR;

    if (ngx_http_yy_sec_waf_init_tfns_in_hash(cf,
            &rule_engine->tfns_in_hash) == NGX_ERROR)
        return NGX_ERROR;

    return NGX_OK;
}

