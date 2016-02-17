/* @(#) $Id: ./src/analysisd/fts.c, 2011/09/08 dcid Exp $
 */

/* Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 *
 * License details at the LICENSE file included with OSSEC or 
 * online at: http://www.ossec.net/en/licensing.html
 */


/* First time seen functions 
 */


#include "fts.h"
#include "eventinfo.h"

int fts_minsize_for_str = 0;

OSList *fts_list = NULL;
OSHash *fts_store = NULL;

FILE *fp_list = NULL;
FILE *fp_ignore = NULL;


/** int FTS_Init()
 * Starts the FTS module.
 */
int FTS_Init()
{
    int fts_list_size;
    char _line[OS_FLSIZE + 1];

    _line[OS_FLSIZE] = '\0';
 
    #ifdef TESTRULE
    return(1);
    #endif           
    
    fts_list = OSList_Create();
    if(!fts_list)
    {
        merror(LIST_ERROR, ARGV0);
        return(0);
    }

    /* Creating store data */
    fts_store = OSHash_Create();
    if(!fts_store)
    {
        merror(LIST_ERROR, ARGV0);
        return(0);
    }
    if(!OSHash_setSize(fts_store, 2048))
    {
        merror(LIST_ERROR, ARGV0);
        return(0);
    }
    

    /* Getting default list size */
    fts_list_size = getDefine_Int("analysisd",
                                  "fts_list_size",
                                  12,512);

    /* Getting minimum string size */
    fts_minsize_for_str = getDefine_Int("analysisd",
                                        "fts_min_size_for_str",
                                        6, 128);
    
    if(!OSList_SetMaxSize(fts_list, fts_list_size))
    {
        merror(LIST_SIZE_ERROR, ARGV0);
        return(0);
    }


    /* creating fts list */
    fp_list = fopen(FTS_QUEUE, "r+");
    if(!fp_list)
    {
        /* Create the file if we cant open it */
        fp_list = fopen(FTS_QUEUE, "w+");
        if(fp_list)
            fclose(fp_list);
        
        chmod(FTS_QUEUE, 0777);
        fp_list = fopen(FTS_QUEUE, "r+");
        if(!fp_list)
        {
            merror(FOPEN_ERROR, ARGV0, FTS_QUEUE);
            return(0);
        }
    }


    /* Adding content from the files to memory */
    fseek(fp_list, 0, SEEK_SET);
    while(fgets(_line, OS_FLSIZE , fp_list) != NULL)
    {
        char *tmp_s;

        /* Removing new lines */
        tmp_s = strchr(_line, '\n');
        if(tmp_s)
        {
            *tmp_s = '\0';
        }


        os_strdup(_line, tmp_s);
        if(OSHash_Add(fts_store, tmp_s, tmp_s) <= 0)
        {
            free(tmp_s);
            merror(LIST_ADD_ERROR, ARGV0);
        }
    }

    
    /* Creating ignore list */
    fp_ignore = fopen(IG_QUEUE, "r+");
    if(!fp_ignore)
    {
        /* Create the file if we cant open it */
        fp_ignore = fopen(IG_QUEUE, "w+");
        if(fp_ignore)
            fclose(fp_ignore);
        
        chmod(IG_QUEUE, 0777);
        fp_ignore = fopen(IG_QUEUE, "r+");
        if(!fp_ignore)
        {
            merror(FOPEN_ERROR, ARGV0, IG_QUEUE);
            return(0);
        }
    }

    debug1("%s: DEBUG: FTSInit completed.", ARGV0);
                                                            
    return(1);
}

/* AddtoIGnore -- adds a pattern to be ignored.
 */
void AddtoIGnore(Eventinfo *lf)
{
    fseek(fp_ignore, 0, SEEK_END);    

    #ifdef TESTRULE
    return;
    #endif
    
    /* Assigning the values to the FTS */
    fprintf(fp_ignore, "%s %s %s %s %s %s %s %s\n",
            (lf->decoder_info->name && (lf->generated_rule->ignore & FTS_NAME))?
                        lf->decoder_info->name:"",
            (lf->id && (lf->generated_rule->ignore & FTS_ID))?lf->id:"",
            (lf->dstuser&&(lf->generated_rule->ignore & FTS_DSTUSER))?
                        lf->dstuser:"",
            (lf->srcip && (lf->generated_rule->ignore & FTS_SRCIP))?
                        lf->srcip:"",
            (lf->dstip && (lf->generated_rule->ignore & FTS_DSTIP))?
                        lf->dstip:"",
            (lf->data && (lf->generated_rule->ignore & FTS_DATA))?
                        lf->data:"",            
            (lf->systemname && (lf->generated_rule->ignore & FTS_SYSTEMNAME))?
                        lf->systemname:"",            
            (lf->generated_rule->ignore & FTS_LOCATION)?lf->location:"");

    fflush(fp_ignore);

    return;
}


/* IGnore v0.1
 * Check if the event is to be ignored.
 * Only after an event is matched (generated_rule must be set).
 */
int IGnore(Eventinfo *lf)
{
    char _line[OS_FLSIZE + 1];
    char _fline[OS_FLSIZE +1];

    _line[OS_FLSIZE] = '\0';


    /* Assigning the values to the FTS */
    snprintf(_line,OS_FLSIZE, "%s %s %s %s %s %s %s %s\n",
            (lf->decoder_info->name && (lf->generated_rule->ckignore & FTS_NAME))?
                            lf->decoder_info->name:"",
            (lf->id && (lf->generated_rule->ckignore & FTS_ID))?lf->id:"",
            (lf->dstuser && (lf->generated_rule->ckignore & FTS_DSTUSER))?
                            lf->dstuser:"",
            (lf->srcip && (lf->generated_rule->ckignore & FTS_SRCIP))?
                            lf->srcip:"",
            (lf->dstip && (lf->generated_rule->ckignore & FTS_DSTIP))?
                            lf->dstip:"",
            (lf->data && (lf->generated_rule->ignore & FTS_DATA))?
                            lf->data:"",
            (lf->systemname && (lf->generated_rule->ignore & FTS_SYSTEMNAME))?
                            lf->systemname:"",                                
            (lf->generated_rule->ckignore & FTS_LOCATION)?lf->location:"");

    _fline[OS_FLSIZE] = '\0';


    /** Checking if the ignore is present **/
    /* Pointing to the beginning of the file */
    fseek(fp_ignore, 0, SEEK_SET);
    while(fgets(_fline, OS_FLSIZE , fp_ignore) != NULL)
    {
        if(strcmp(_fline, _line) != 0)
            continue;

        /* If we match, we can return 1 */
        return(1);
    }

    return(0);
}


/* FTS v0.1
 *  Check if the word "msg" is present on the "queue".
 *  If it is not, write it there.
 */ 
int FTS(Eventinfo *lf)
{
    int number_of_matches = 0;

    char _line[OS_FLSIZE + 1];
    
    char *line_for_list = NULL;

    OSListNode *fts_node;

    #ifdef TESTRULE
    return(0);
    #endif

    _line[OS_FLSIZE] = '\0';


    /* Assigning the values to the FTS */
    snprintf(_line, OS_FLSIZE, "%s %s %s %s %s %s %s %s %s",
            lf->decoder_info->name,
            (lf->id && (lf->decoder_info->fts & FTS_ID))?lf->id:"",
            (lf->dstuser && (lf->decoder_info->fts & FTS_DSTUSER))?lf->dstuser:"",
            (lf->srcuser && (lf->decoder_info->fts & FTS_SRCUSER))?lf->srcuser:"",
            (lf->srcip && (lf->decoder_info->fts & FTS_SRCIP))?lf->srcip:"",
            (lf->dstip && (lf->decoder_info->fts & FTS_DSTIP))?lf->dstip:"",
            (lf->data && (lf->decoder_info->fts & FTS_DATA))?lf->data:"",
            (lf->systemname && (lf->decoder_info->fts & FTS_SYSTEMNAME))?lf->systemname:"",
            (lf->decoder_info->fts & FTS_LOCATION)?lf->location:"");


    /** Checking if FTS is already present **/
    if(OSHash_Get(fts_store, _line))
    {
        return(0);
    }        

    
    /* Checking if from the last FTS events, we had
     * at least 3 "similars" before. If yes, we just
     * ignore it.
     */
    if(lf->decoder_info->type == IDS)
    {
        fts_node = OSList_GetLastNode(fts_list);
        while(fts_node)
        {
            if(OS_StrHowClosedMatch((char *)fts_node->data, _line) > 
                    fts_minsize_for_str)
            {
                number_of_matches++;

                /* We go and add this new entry to the list */
                if(number_of_matches > 2)
                {
                    _line[fts_minsize_for_str] = '\0';
                    break;
                }
            }

            fts_node = OSList_GetPrevNode(fts_list);
        }

        os_strdup(_line, line_for_list);
        OSList_AddData(fts_list, line_for_list);
    }
    
    
    /* Storing new entry */
    if(line_for_list == NULL)
    {
        os_strdup(_line, line_for_list);
    }

    if(OSHash_Add(fts_store, line_for_list, line_for_list) <= 1)
    {
        return(0);
    }

    
    /* Saving to fts fp */	
    fseek(fp_list, 0, SEEK_END);
    fprintf(fp_list,"%s\n", _line);
    fflush(fp_list);

    return(1);
}


/* EOF */